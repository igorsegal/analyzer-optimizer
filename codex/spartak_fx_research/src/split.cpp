#include "spartak/split.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace spartak {

void ChronologicalSplit::validate(const std::size_t total_bars) const {
    const auto valid_range = [total_bars](const IndexRange range) {
        return range.begin <= range.end && range.end <= total_bars;
    };
    if (!valid_range(in_sample)
        || !valid_range(validation)
        || !valid_range(blind_oos)) {
        throw std::invalid_argument("chronological split contains an invalid range");
    }
    if (in_sample.empty() || validation.empty() || blind_oos.empty()) {
        throw std::invalid_argument("all three chronological partitions must be non-empty");
    }
    if (in_sample.begin != 0 || blind_oos.end != total_bars) {
        throw std::invalid_argument("chronological split must cover from first to last bar");
    }
    if (in_sample.end + embargo_bars != validation.begin
        || validation.end + embargo_bars != blind_oos.begin) {
        throw std::invalid_argument("chronological split embargo boundaries are inconsistent");
    }
}

ChronologicalSplit make_chronological_split(
    const std::size_t total_bars,
    const SplitCounts& counts) {
    if (counts.in_sample_bars == 0
        || counts.validation_bars == 0
        || counts.blind_oos_bars == 0) {
        throw std::invalid_argument("split counts must make all partitions non-empty");
    }
    if (counts.embargo_bars > (std::numeric_limits<std::size_t>::max() / 2)) {
        throw std::overflow_error("split embargo is too large");
    }
    const std::size_t embargo_total = counts.embargo_bars * 2;
    if (counts.in_sample_bars > total_bars
        || counts.validation_bars > total_bars - counts.in_sample_bars
        || embargo_total > total_bars - counts.in_sample_bars - counts.validation_bars
        || counts.blind_oos_bars
            != total_bars - counts.in_sample_bars - counts.validation_bars - embargo_total) {
        throw std::invalid_argument(
            "explicit split counts plus two embargoes must equal total bars exactly");
    }

    const IndexRange in_sample{0, counts.in_sample_bars};
    const std::size_t validation_begin = in_sample.end + counts.embargo_bars;
    const IndexRange validation{
        validation_begin,
        validation_begin + counts.validation_bars};
    const std::size_t oos_begin = validation.end + counts.embargo_bars;
    const IndexRange blind_oos{oos_begin, oos_begin + counts.blind_oos_bars};

    ChronologicalSplit split{in_sample, validation, blind_oos, counts.embargo_bars};
    split.validate(total_bars);
    return split;
}

ChronologicalSplit make_chronological_split(
    const std::size_t total_bars,
    const SplitFractions& fractions) {
    if (!std::isfinite(fractions.in_sample_fraction)
        || !std::isfinite(fractions.validation_fraction)
        || fractions.in_sample_fraction <= 0.0
        || fractions.validation_fraction <= 0.0
        || fractions.in_sample_fraction + fractions.validation_fraction >= 1.0) {
        throw std::invalid_argument(
            "IS and validation fractions must be positive and sum to less than one");
    }
    if (fractions.minimum_bars_per_partition == 0) {
        throw std::invalid_argument("minimum bars per partition must be positive");
    }
    if (fractions.embargo_bars > total_bars / 2) {
        throw std::invalid_argument("embargoes leave no usable observations");
    }

    const std::size_t usable = total_bars - 2 * fractions.embargo_bars;
    const auto in_sample_bars = static_cast<std::size_t>(
        std::floor(static_cast<double>(usable) * fractions.in_sample_fraction));
    const auto validation_bars = static_cast<std::size_t>(
        std::floor(static_cast<double>(usable) * fractions.validation_fraction));
    if (in_sample_bars > usable || validation_bars > usable - in_sample_bars) {
        throw std::overflow_error("fractional split calculation overflowed");
    }
    const std::size_t blind_oos_bars = usable - in_sample_bars - validation_bars;
    if (in_sample_bars < fractions.minimum_bars_per_partition
        || validation_bars < fractions.minimum_bars_per_partition
        || blind_oos_bars < fractions.minimum_bars_per_partition) {
        throw std::invalid_argument("fractional split violates minimum partition size");
    }

    return make_chronological_split(
        total_bars,
        SplitCounts{
            in_sample_bars,
            validation_bars,
            blind_oos_bars,
            fractions.embargo_bars});
}

}  // namespace spartak
