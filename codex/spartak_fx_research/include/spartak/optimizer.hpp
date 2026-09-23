#pragma once

#include "spartak/backtest.hpp"

#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace spartak {

struct ParameterAxis {
    std::string name;
    std::vector<double> values;
};

struct IsValidationSeries {
    std::string instrument;
    std::span<const MarketBar> in_sample;
    std::span<const MarketBar> validation;
};

struct BlindOosSeries {
    std::string instrument;
    std::span<const MarketBar> blind_oos;
};

using ObjectiveFunction = std::function<double(const BacktestResult&)>;

struct GridSearchConfig {
    std::vector<ParameterAxis> axes;
    std::size_t top_k{50};
    std::size_t minimum_in_sample_trades{};
    std::size_t minimum_validation_trades{};
};

struct RankedCandidate {
    std::size_t validation_rank{};
    std::string instrument;
    StrategyParameters parameters;
    PerformanceSummary in_sample;
    PerformanceSummary validation;
    double in_sample_score{};
    double validation_score{};
};

class LockedLeaderboard {
public:
    [[nodiscard]] const std::vector<RankedCandidate>& rows() const noexcept { return rows_; }
    [[nodiscard]] const std::string& lock_id() const noexcept { return lock_id_; }

private:
    std::vector<RankedCandidate> rows_;
    std::string lock_id_;

    LockedLeaderboard(std::vector<RankedCandidate> rows, std::string lock_id);
    friend class GridOptimizer;
};

struct OosAuditRow {
    std::size_t locked_validation_rank{};
    std::string instrument;
    StrategyParameters parameters;
    PerformanceSummary blind_oos;
    double blind_oos_score{};
    std::string leaderboard_lock_id;
};

class GridOptimizer {
public:
    [[nodiscard]] std::vector<StrategyParameters> expand_grid(
        const StrategyParameters& base,
        std::span<const ParameterAxis> axes) const;

    // Produces at most one winning parameter set per instrument, ranks solely
    // on validation, truncates to top_k, and returns an immutable lock object.
    [[nodiscard]] LockedLeaderboard optimize_and_lock(
        std::span<const IsValidationSeries> series,
        const IStrategy& strategy,
        const StrategyParameters& base,
        const BacktestConfig& backtest_config,
        const GridSearchConfig& search_config,
        const ObjectiveFunction& objective) const;

    // The audit preserves validation rank. It never re-sorts or selects using
    // blind OOS outcomes.
    [[nodiscard]] std::vector<OosAuditRow> audit_locked_oos(
        const LockedLeaderboard& locked,
        std::span<const BlindOosSeries> blind_series,
        const IStrategy& strategy,
        const BacktestConfig& backtest_config,
        const ObjectiveFunction& objective) const;
};

}  // namespace spartak
