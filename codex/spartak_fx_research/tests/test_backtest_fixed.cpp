#include "spartak/backtest.hpp"
#include "spartak/optimizer.hpp"
#include "spartak/split.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

using namespace spartak;

[[nodiscard]] bool near(const double left, const double right, const double tolerance = 1.0e-9) {
    return std::abs(left - right) <= tolerance;
}

[[nodiscard]] StrategyParameters locked_test_parameters() {
    StrategyParameters parameters;
    parameters.source_pending = false;
    parameters.rule_revision = "unit_test_only";
    return parameters;
}

[[nodiscard]] BacktestConfig test_config() {
    BacktestConfig config;
    config.risk.initial_equity = 10'000.0;
    config.risk.risk_fraction_per_trade = 0.01;
    config.risk.max_gross_leverage = 100.0;
    return config;
}

class OneShotStrategy final : public IStrategy {
public:
    OneShotStrategy(const Timestamp signal_time, TradeSignal signal)
        : signal_time_(signal_time), signal_(std::move(signal)) {}

    std::optional<TradeSignal> on_bar_close(
        const std::span<const MarketBar> bars,
        const std::size_t bar_index,
        const StrategyParameters&) const override {
        if (bars[bar_index].timestamp == signal_time_) {
            return signal_;
        }
        return std::nullopt;
    }

private:
    Timestamp signal_time_{};
    TradeSignal signal_;
};

class DirectionalGridStrategy final : public IStrategy {
public:
    std::optional<TradeSignal> on_bar_close(
        const std::span<const MarketBar> bars,
        const std::size_t bar_index,
        const StrategyParameters& parameters) const override {
        if (bar_index != 0) {
            return std::nullopt;
        }
        const double direction = parameters.require_numeric("direction");
        const double close = bars[bar_index].close;
        if (direction > 0.0) {
            return TradeSignal{
                PositionSide::long_position,
                close - 1.0,
                close + 1.0,
                "unit_test_long"};
        }
        return TradeSignal{
            PositionSide::short_position,
            close + 1.0,
            close - 1.0,
            "unit_test_short"};
    }
};

void test_next_open_spread_slippage_fees_and_risk_sizing() {
    const std::vector<MarketBar> bars{
        {1, 100.0, 100.0, 100.0, 100.0, std::nullopt},
        {2, 100.0, 102.0, 99.0, 101.0, std::nullopt},
    };
    OneShotStrategy strategy{
        1,
        TradeSignal{
            PositionSide::long_position,
            98.0,
            101.0,
            "next_open_test"}};
    auto config = test_config();
    config.execution.default_spread = 0.2;
    config.execution.slippage_per_fill = 0.05;
    config.execution.fees.fixed_per_order = 1.0;
    config.execution.fees.proportional_bps = 10.0;

    const auto result = BacktestEngine{}.run(
        bars,
        strategy,
        locked_test_parameters(),
        config);
    assert(result.trades.size() == 1);
    const auto& trade = result.trades.front();
    assert(trade.signal_timestamp == 1);
    assert(trade.entry_timestamp == 2);
    assert(trade.exit_timestamp == 2);
    assert(trade.exit_reason == ExitReason::take_profit);
    assert(!trade.same_bar_ambiguity);
    assert(near(trade.entry_price, 100.15));
    assert(near(trade.exit_price, 100.95));

    const double risk_per_unit = (100.15 - 97.95) + 0.001 * (100.15 + 97.95);
    const double expected_quantity = (100.0 - 2.0) / risk_per_unit;
    assert(near(trade.quantity, expected_quantity));
    const double expected_fees = 2.0
        + expected_quantity * 0.001 * (100.15 + 100.95);
    const double expected_net = expected_quantity * (100.95 - 100.15) - expected_fees;
    assert(near(trade.fees, expected_fees));
    assert(near(trade.net_pnl, expected_net));
    assert(near(result.summary.final_equity, 10'000.0 + expected_net));
}

void test_pessimistic_same_bar_ambiguity() {
    const std::vector<MarketBar> bars{
        {1, 100.0, 100.0, 100.0, 100.0, std::nullopt},
        {2, 100.0, 102.0, 98.0, 100.0, std::nullopt},
    };
    OneShotStrategy strategy{
        1,
        TradeSignal{
            PositionSide::long_position,
            99.0,
            101.0,
            "ambiguous_bar_test"}};

    const auto result = BacktestEngine{}.run(
        bars,
        strategy,
        locked_test_parameters(),
        test_config());
    assert(result.trades.size() == 1);
    const auto& trade = result.trades.front();
    assert(trade.exit_reason == ExitReason::stop_loss);
    assert(trade.same_bar_ambiguity);
    assert(near(trade.quantity, 100.0));
    assert(near(trade.net_pnl, -100.0));
    assert(near(result.summary.final_equity, 9'900.0));
}

void test_source_pending_is_not_executable() {
    const std::vector<MarketBar> bars{
        {1, 100.0, 100.0, 100.0, 100.0, std::nullopt},
        {2, 100.0, 100.0, 100.0, 100.0, std::nullopt},
    };
    OneShotStrategy strategy{
        1,
        TradeSignal{PositionSide::long_position, 99.0, 101.0, "blocked"}};
    bool threw = false;
    try {
        (void)BacktestEngine{}.run(bars, strategy, StrategyParameters{}, test_config());
    } catch (const std::logic_error&) {
        threw = true;
    }
    assert(threw);
}

void test_chronological_split_with_embargo() {
    const auto split = make_chronological_split(
        15,
        SplitCounts{5, 4, 4, 1});
    assert(split.in_sample.begin == 0 && split.in_sample.end == 5);
    assert(split.validation.begin == 6 && split.validation.end == 10);
    assert(split.blind_oos.begin == 11 && split.blind_oos.end == 15);
    split.validate(15);
}

void test_validation_lock_and_oos_audit_preserve_rank() {
    const std::vector<MarketBar> up_is{
        {1, 100.0, 100.0, 100.0, 100.0, std::nullopt},
        {2, 100.0, 101.5, 99.8, 101.2, std::nullopt},
    };
    const std::vector<MarketBar> up_validation{
        {10, 100.0, 100.0, 100.0, 100.0, std::nullopt},
        {11, 100.0, 101.5, 99.8, 101.2, std::nullopt},
    };
    const std::vector<MarketBar> down_oos{
        {20, 100.0, 100.0, 100.0, 100.0, std::nullopt},
        {21, 100.0, 100.2, 98.5, 98.8, std::nullopt},
    };

    StrategyParameters base = locked_test_parameters();
    GridSearchConfig search;
    search.axes.push_back(ParameterAxis{"direction", {-1.0, 1.0}});
    search.top_k = 50;
    search.minimum_in_sample_trades = 1;
    search.minimum_validation_trades = 1;
    const auto objective = [](const BacktestResult& result) {
        return result.summary.net_pnl;
    };
    const DirectionalGridStrategy strategy;
    const std::vector<IsValidationSeries> train{
        {"GBPJPY", std::span<const MarketBar>{up_is}, std::span<const MarketBar>{up_validation}},
    };

    const auto locked = GridOptimizer{}.optimize_and_lock(
        train,
        strategy,
        base,
        test_config(),
        search,
        objective);
    assert(locked.rows().size() == 1);
    assert(locked.rows()[0].validation_rank == 1);
    assert(locked.rows()[0].parameters.require_numeric("direction") == 1.0);
    assert(!locked.lock_id().empty());

    const std::vector<BlindOosSeries> blind{
        {"GBPJPY", std::span<const MarketBar>{down_oos}},
    };
    const auto audit = GridOptimizer{}.audit_locked_oos(
        locked,
        blind,
        strategy,
        test_config(),
        objective);
    assert(audit.size() == 1);
    assert(audit[0].locked_validation_rank == 1);
    assert(audit[0].leaderboard_lock_id == locked.lock_id());
    assert(audit[0].blind_oos.net_pnl < 0.0);
}

}  // namespace

int main() {
    test_next_open_spread_slippage_fees_and_risk_sizing();
    test_pessimistic_same_bar_ambiguity();
    test_source_pending_is_not_executable();
    test_chronological_split_with_embargo();
    test_validation_lock_and_oos_audit_preserve_rank();
    std::cout << "All Spartak backtest tests passed.\n";
    return 0;
}
