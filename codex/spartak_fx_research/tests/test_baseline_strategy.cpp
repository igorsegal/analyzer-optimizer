#include "spartak/baseline_strategy.hpp"

#include <cmath>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using spartak::MarketBar;
using spartak::PositionSide;
using spartak::SpartakPriceBaselineStrategy;
using spartak::StrategyParameters;

[[nodiscard]] MarketBar bar(
    const std::int64_t time,
    const double open,
    const double high,
    const double low,
    const double close) {
    return MarketBar{time, open, high, low, close, 0.02};
}

[[nodiscard]] StrategyParameters parameters() {
    StrategyParameters value;
    value.source_pending = false;
    value.rule_revision = "machine_hypothesis_v0_1_test";
    value.numeric = {
        {"atr_period", 3.0},
        {"accumulation_bars", 3.0},
        {"retest_max_bars", 4.0},
        {"accumulation_max_range_atr", 3.0},
        {"minimum_overlap_ratio", 0.25},
        {"breakout_min_atr", 0.20},
        {"retest_tolerance_atr", 0.20},
        {"stop_buffer_atr", 0.10},
        {"target_r", 2.0},
    };
    value.boolean = {
        {"enable_long", true},
        {"enable_short", true},
        {"use_target", true},
    };
    return value;
}

void require(const bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void test_long_signal_and_no_premature_signal() {
    const std::vector<MarketBar> bars{
        bar(0, 100.0, 100.8, 99.4, 100.1),
        bar(1, 100.1, 100.7, 99.6, 100.0),
        bar(2, 100.0, 100.5, 99.7, 100.2),
        bar(3, 100.2, 100.6, 99.8, 100.1),
        bar(4, 100.1, 100.5, 99.9, 100.2),
        bar(5, 100.2, 102.0, 100.1, 101.7),
        bar(6, 101.4, 101.5, 100.35, 100.7),
        bar(7, 100.7, 101.8, 100.6, 101.5),
    };
    SpartakPriceBaselineStrategy strategy;
    const auto p = parameters();
    const auto premature = strategy.on_bar_close(
        std::span<const MarketBar>{bars}.first(7),
        6,
        p);
    require(!premature, "long signal appeared before confirmation");
    const auto signal = strategy.on_bar_close(bars, 7, p);
    require(signal.has_value(), "expected a long baseline signal");
    require(signal->side == PositionSide::long_position, "wrong long signal side");
    require(signal->stop_loss < bars.back().close, "long stop is not protective");
    require(signal->take_profit && *signal->take_profit > bars.back().close,
        "long target is not above price");
}

void test_short_signal() {
    const std::vector<MarketBar> bars{
        bar(0, 100.0, 100.8, 99.4, 100.1),
        bar(1, 100.1, 100.7, 99.6, 100.0),
        bar(2, 100.0, 100.4, 99.6, 99.9),
        bar(3, 99.9, 100.3, 99.5, 99.8),
        bar(4, 99.8, 100.2, 99.6, 99.9),
        bar(5, 99.9, 100.0, 97.8, 98.2),
        bar(6, 98.5, 99.65, 98.4, 99.3),
        bar(7, 99.3, 99.4, 98.0, 98.3),
    };
    SpartakPriceBaselineStrategy strategy;
    const auto signal = strategy.on_bar_close(bars, 7, parameters());
    require(signal.has_value(), "expected a short baseline signal");
    require(signal->side == PositionSide::short_position, "wrong short signal side");
    require(signal->stop_loss > bars.back().close, "short stop is not protective");
    require(signal->take_profit && *signal->take_profit < bars.back().close,
        "short target is not below price");
}

void test_causal_span_contract() {
    const std::vector<MarketBar> bars{
        bar(0, 10.0, 10.2, 9.8, 10.0),
        bar(1, 10.0, 10.2, 9.8, 10.0),
        bar(2, 10.0, 10.2, 9.8, 10.0),
        bar(3, 10.0, 10.2, 9.8, 10.0),
        bar(4, 10.0, 10.2, 9.8, 10.0),
    };
    SpartakPriceBaselineStrategy strategy;
    bool threw = false;
    try {
        (void)strategy.on_bar_close(bars, 3, parameters());
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    require(threw, "strategy accepted future bars beyond bar_index");
}

void test_invalid_parameters_fail_closed() {
    const std::vector<MarketBar> bars{
        bar(0, 10.0, 10.2, 9.8, 10.0),
        bar(1, 10.0, 10.2, 9.8, 10.0),
        bar(2, 10.0, 10.2, 9.8, 10.0),
        bar(3, 10.0, 10.2, 9.8, 10.0),
        bar(4, 10.0, 10.2, 9.8, 10.0),
    };
    auto p = parameters();
    p.numeric["accumulation_bars"] = 2.5;
    SpartakPriceBaselineStrategy strategy;
    bool threw = false;
    try {
        (void)strategy.on_bar_close(bars, 4, p);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    require(threw, "fractional bar-count parameter did not fail closed");
}

}  // namespace

int main() {
    try {
        test_long_signal_and_no_premature_signal();
        test_short_signal();
        test_causal_span_contract();
        test_invalid_parameters_fail_closed();
        std::cout << "baseline strategy tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "baseline strategy test failure: " << error.what() << '\n';
        return 1;
    }
}
