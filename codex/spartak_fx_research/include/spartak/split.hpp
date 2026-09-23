#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>

namespace spartak {

struct IndexRange {
    std::size_t begin{};
    std::size_t end{};

    [[nodiscard]] constexpr std::size_t size() const noexcept { return end - begin; }
    [[nodiscard]] constexpr bool empty() const noexcept { return begin == end; }
};

struct ChronologicalSplit {
    IndexRange in_sample;
    IndexRange validation;
    IndexRange blind_oos;
    std::size_t embargo_bars{};

    void validate(std::size_t total_bars) const;
};

struct SplitCounts {
    std::size_t in_sample_bars{};
    std::size_t validation_bars{};
    std::size_t blind_oos_bars{};
    std::size_t embargo_bars{};
};

struct SplitFractions {
    double in_sample_fraction{};
    double validation_fraction{};
    std::size_t embargo_bars{};
    std::size_t minimum_bars_per_partition{1};
};

[[nodiscard]] ChronologicalSplit make_chronological_split(
    std::size_t total_bars,
    const SplitCounts& counts);

[[nodiscard]] ChronologicalSplit make_chronological_split(
    std::size_t total_bars,
    const SplitFractions& fractions);

template <typename T>
[[nodiscard]] std::span<const T> slice(
    std::span<const T> values,
    const IndexRange range) {
    if (range.begin > range.end || range.end > values.size()) {
        throw std::out_of_range("split range is outside the source span");
    }
    return values.subspan(range.begin, range.size());
}

}  // namespace spartak
