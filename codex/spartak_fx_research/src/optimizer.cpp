#include "spartak/optimizer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace spartak {
namespace {

[[nodiscard]] bool candidate_is_better(
    const RankedCandidate& left,
    const RankedCandidate& right) {
    if (left.validation_score != right.validation_score) {
        return left.validation_score > right.validation_score;
    }
    if (left.in_sample_score != right.in_sample_score) {
        return left.in_sample_score > right.in_sample_score;
    }
    const std::string left_key = canonical_parameter_key(left.parameters);
    const std::string right_key = canonical_parameter_key(right.parameters);
    if (left.instrument != right.instrument) {
        return left.instrument < right.instrument;
    }
    return left_key < right_key;
}

[[nodiscard]] std::string make_lock_id(const std::vector<RankedCandidate>& rows) {
    std::ostringstream serialized;
    serialized << std::setprecision(std::numeric_limits<double>::max_digits10);
    for (const auto& row : rows) {
        serialized << row.validation_rank << '|'
                   << row.instrument << '|'
                   << canonical_parameter_key(row.parameters) << '|'
                   << row.in_sample_score << '|'
                   << row.validation_score << '\n';
    }

    constexpr std::uint64_t offset = 14695981039346656037ULL;
    constexpr std::uint64_t prime = 1099511628211ULL;
    std::uint64_t hash = offset;
    for (const unsigned char byte : serialized.str()) {
        hash ^= byte;
        hash *= prime;
    }
    std::ostringstream encoded;
    encoded << std::hex << std::setfill('0') << std::setw(16) << hash;
    return encoded.str();
}

}  // namespace

LockedLeaderboard::LockedLeaderboard(
    std::vector<RankedCandidate> rows,
    std::string lock_id)
    : rows_(std::move(rows)), lock_id_(std::move(lock_id)) {}

std::vector<StrategyParameters> GridOptimizer::expand_grid(
    const StrategyParameters& base,
    const std::span<const ParameterAxis> axes) const {
    std::set<std::string, std::less<>> names;
    std::vector<StrategyParameters> grid{base};

    for (const auto& axis : axes) {
        if (axis.name.empty() || axis.values.empty()) {
            throw std::invalid_argument("every grid axis needs a name and at least one value");
        }
        if (!names.insert(axis.name).second) {
            throw std::invalid_argument("grid axis names must be unique");
        }
        for (const double value : axis.values) {
            if (!std::isfinite(value)) {
                throw std::invalid_argument("grid values must be finite");
            }
        }
        if (grid.size() > grid.max_size() / axis.values.size()) {
            throw std::length_error("parameter grid is too large");
        }

        std::vector<StrategyParameters> expanded;
        expanded.reserve(grid.size() * axis.values.size());
        for (const auto& parameters : grid) {
            for (const double value : axis.values) {
                auto candidate = parameters;
                candidate.numeric[axis.name] = value;
                expanded.push_back(std::move(candidate));
            }
        }
        grid = std::move(expanded);
    }
    return grid;
}

LockedLeaderboard GridOptimizer::optimize_and_lock(
    const std::span<const IsValidationSeries> series,
    const IStrategy& strategy,
    const StrategyParameters& base,
    const BacktestConfig& backtest_config,
    const GridSearchConfig& search_config,
    const ObjectiveFunction& objective) const {
    if (series.empty()) {
        throw std::invalid_argument("optimizer needs at least one instrument");
    }
    if (!objective) {
        throw std::invalid_argument("optimizer objective must be supplied explicitly");
    }
    if (search_config.top_k == 0) {
        throw std::invalid_argument("top_k must be positive");
    }
    base.validate_for_execution();
    backtest_config.validate();
    const auto grid = expand_grid(base, search_config.axes);
    BacktestEngine engine;

    std::set<std::string, std::less<>> instruments;
    std::vector<RankedCandidate> instrument_winners;
    instrument_winners.reserve(series.size());

    for (const auto& dataset : series) {
        if (dataset.instrument.empty()
            || dataset.in_sample.empty()
            || dataset.validation.empty()) {
            throw std::invalid_argument("each instrument needs named, non-empty IS and validation data");
        }
        if (!instruments.insert(dataset.instrument).second) {
            throw std::invalid_argument("instrument names must be unique in optimizer input");
        }

        std::optional<RankedCandidate> best;
        for (const auto& parameters : grid) {
            const auto is_result = engine.run(
                dataset.in_sample,
                strategy,
                parameters,
                backtest_config);
            if (is_result.summary.trade_count < search_config.minimum_in_sample_trades) {
                continue;
            }
            const auto validation_result = engine.run(
                dataset.validation,
                strategy,
                parameters,
                backtest_config);
            if (validation_result.summary.trade_count < search_config.minimum_validation_trades) {
                continue;
            }

            const double is_score = objective(is_result);
            const double validation_score = objective(validation_result);
            if (!std::isfinite(is_score) || !std::isfinite(validation_score)) {
                throw std::invalid_argument("optimizer objective must return finite scores");
            }

            RankedCandidate candidate{
                0,
                dataset.instrument,
                parameters,
                is_result.summary,
                validation_result.summary,
                is_score,
                validation_score};
            if (!best || candidate_is_better(candidate, *best)) {
                best = std::move(candidate);
            }
        }
        if (best) {
            instrument_winners.push_back(std::move(*best));
        }
    }

    std::sort(
        instrument_winners.begin(),
        instrument_winners.end(),
        candidate_is_better);
    if (instrument_winners.size() > search_config.top_k) {
        instrument_winners.resize(search_config.top_k);
    }
    for (std::size_t index = 0; index < instrument_winners.size(); ++index) {
        instrument_winners[index].validation_rank = index + 1;
    }
    std::string lock_id = make_lock_id(instrument_winners);
    return LockedLeaderboard{std::move(instrument_winners), std::move(lock_id)};
}

std::vector<OosAuditRow> GridOptimizer::audit_locked_oos(
    const LockedLeaderboard& locked,
    const std::span<const BlindOosSeries> blind_series,
    const IStrategy& strategy,
    const BacktestConfig& backtest_config,
    const ObjectiveFunction& objective) const {
    if (!objective) {
        throw std::invalid_argument("OOS audit objective must be supplied explicitly");
    }
    backtest_config.validate();

    std::map<std::string, std::span<const MarketBar>, std::less<>> by_instrument;
    for (const auto& dataset : blind_series) {
        if (dataset.instrument.empty() || dataset.blind_oos.empty()) {
            throw std::invalid_argument("each blind OOS series must be named and non-empty");
        }
        if (!by_instrument.emplace(dataset.instrument, dataset.blind_oos).second) {
            throw std::invalid_argument("blind OOS instrument names must be unique");
        }
    }

    BacktestEngine engine;
    std::vector<OosAuditRow> audit;
    audit.reserve(locked.rows().size());
    for (const auto& candidate : locked.rows()) {
        const auto found = by_instrument.find(candidate.instrument);
        if (found == by_instrument.end()) {
            throw std::invalid_argument(
                "missing blind OOS series for locked instrument: " + candidate.instrument);
        }
        const auto result = engine.run(
            found->second,
            strategy,
            candidate.parameters,
            backtest_config);
        const double score = objective(result);
        if (!std::isfinite(score)) {
            throw std::invalid_argument("OOS audit objective must return a finite score");
        }
        audit.push_back(OosAuditRow{
            candidate.validation_rank,
            candidate.instrument,
            candidate.parameters,
            result.summary,
            score,
            locked.lock_id()});
    }
    return audit;
}

}  // namespace spartak
