#include "spartak/baseline_strategy.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace spartak {
namespace {

struct BaselineParameters {
    std::size_t atr_period{};
    std::size_t accumulation_bars{};
    std::size_t retest_max_bars{};
    double accumulation_max_range_atr{};
    double minimum_overlap_ratio{};
    double breakout_min_atr{};
    double retest_tolerance_atr{};
    double stop_buffer_atr{};
    double target_r{};
    bool enable_long{};
    bool enable_short{};
    bool use_target{};
};

[[nodiscard]] std::size_t require_count(
    const StrategyParameters& parameters,
    const std::string_view name,
    const std::size_t minimum,
    const std::size_t maximum) {
    const double value = parameters.require_numeric(name);
    if (!std::isfinite(value)
        || value < static_cast<double>(minimum)
        || value > static_cast<double>(maximum)
        || std::floor(value) != value) {
        throw std::invalid_argument(
            std::string{name} + " must be an integer in the declared safe range");
    }
    return static_cast<std::size_t>(value);
}

[[nodiscard]] double require_range(
    const StrategyParameters& parameters,
    const std::string_view name,
    const double minimum,
    const double maximum,
    const bool minimum_is_inclusive = true) {
    const double value = parameters.require_numeric(name);
    const bool below_minimum = minimum_is_inclusive ? value < minimum : value <= minimum;
    if (!std::isfinite(value) || below_minimum || value > maximum) {
        throw std::invalid_argument(std::string{name} + " is outside its safe range");
    }
    return value;
}

[[nodiscard]] BaselineParameters load_parameters(const StrategyParameters& parameters) {
    parameters.validate_for_execution();
    BaselineParameters result;
    result.atr_period = require_count(parameters, "atr_period", 2, 10000);
    result.accumulation_bars = require_count(parameters, "accumulation_bars", 2, 10000);
    result.retest_max_bars = require_count(parameters, "retest_max_bars", 1, 10000);
    result.accumulation_max_range_atr = require_range(
        parameters, "accumulation_max_range_atr", 0.0, 1000.0, false);
    result.minimum_overlap_ratio = require_range(
        parameters, "minimum_overlap_ratio", 0.0, 1.0);
    result.breakout_min_atr = require_range(
        parameters, "breakout_min_atr", 0.0, 1000.0);
    result.retest_tolerance_atr = require_range(
        parameters, "retest_tolerance_atr", 0.0, 1000.0);
    result.stop_buffer_atr = require_range(
        parameters, "stop_buffer_atr", 0.0, 1000.0);
    result.target_r = require_range(parameters, "target_r", 0.0, 1000.0, false);
    result.enable_long = parameters.require_boolean("enable_long");
    result.enable_short = parameters.require_boolean("enable_short");
    result.use_target = parameters.require_boolean("use_target");
    if (!result.enable_long && !result.enable_short) {
        throw std::invalid_argument("at least one trade direction must be enabled");
    }
    return result;
}

[[nodiscard]] double true_range(
    const std::span<const MarketBar> bars,
    const std::size_t index) {
    const auto& bar = bars[index];
    if (index == 0) {
        return bar.high - bar.low;
    }
    const double previous_close = bars[index - 1].close;
    return std::max({
        bar.high - bar.low,
        std::abs(bar.high - previous_close),
        std::abs(bar.low - previous_close)});
}

[[nodiscard]] double causal_atr(
    const std::span<const MarketBar> bars,
    const std::size_t end_index,
    const std::size_t period) {
    if (end_index + 1 < period) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const std::size_t begin = end_index + 1 - period;
    double total = 0.0;
    for (std::size_t index = begin; index <= end_index; ++index) {
        total += true_range(bars, index);
    }
    return total / static_cast<double>(period);
}

struct Zone {
    double low{};
    double high{};
};

[[nodiscard]] Zone make_zone(
    const std::span<const MarketBar> bars,
    const std::size_t begin,
    const std::size_t end) {
    Zone zone{bars[begin].low, bars[begin].high};
    for (std::size_t index = begin + 1; index < end; ++index) {
        zone.low = std::min(zone.low, bars[index].low);
        zone.high = std::max(zone.high, bars[index].high);
    }
    return zone;
}

[[nodiscard]] bool has_minimum_overlap(
    const std::span<const MarketBar> bars,
    const std::size_t begin,
    const std::size_t end,
    const double minimum_ratio) {
    for (std::size_t index = begin + 1; index < end; ++index) {
        const auto& left = bars[index - 1];
        const auto& right = bars[index];
        const double smaller_range = std::min(
            left.high - left.low,
            right.high - right.low);
        const double overlap = std::max(
            0.0,
            std::min(left.high, right.high) - std::max(left.low, right.low));
        if (!(smaller_range > 0.0) || overlap / smaller_range < minimum_ratio) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool long_confirmation(
    const std::span<const MarketBar> bars,
    const std::size_t index,
    const Zone& zone) {
    if (index == 0) {
        return false;
    }
    const auto& bar = bars[index];
    return bar.close > bar.open
        && bar.close > bars[index - 1].close
        && bar.close > zone.high;
}

[[nodiscard]] bool short_confirmation(
    const std::span<const MarketBar> bars,
    const std::size_t index,
    const Zone& zone) {
    if (index == 0) {
        return false;
    }
    const auto& bar = bars[index];
    return bar.close < bar.open
        && bar.close < bars[index - 1].close
        && bar.close < zone.low;
}

[[nodiscard]] bool long_retest_touch(
    const MarketBar& bar,
    const Zone& zone,
    const double tolerance) {
    return bar.low <= zone.high + tolerance
        && bar.high >= zone.high - tolerance;
}

[[nodiscard]] bool short_retest_touch(
    const MarketBar& bar,
    const Zone& zone,
    const double tolerance) {
    return bar.high >= zone.low - tolerance
        && bar.low <= zone.low + tolerance;
}

[[nodiscard]] std::optional<TradeSignal> evaluate_candidate(
    const std::span<const MarketBar> bars,
    const std::size_t confirmation_index,
    const std::size_t breakout_index,
    const BaselineParameters& parameters) {
    if (breakout_index < parameters.accumulation_bars) {
        return std::nullopt;
    }
    const std::size_t accumulation_begin = breakout_index - parameters.accumulation_bars;
    const Zone zone = make_zone(bars, accumulation_begin, breakout_index);
    const double atr = causal_atr(bars, breakout_index - 1, parameters.atr_period);
    if (!std::isfinite(atr) || !(atr > 0.0)) {
        return std::nullopt;
    }
    const double zone_range = zone.high - zone.low;
    if (!(zone_range > 0.0)
        || zone_range > parameters.accumulation_max_range_atr * atr
        || !has_minimum_overlap(
            bars,
            accumulation_begin,
            breakout_index,
            parameters.minimum_overlap_ratio)) {
        return std::nullopt;
    }

    const auto& breakout = bars[breakout_index];
    const double breakout_distance = parameters.breakout_min_atr * atr;
    const bool long_breakout = parameters.enable_long
        && breakout.close > breakout.open
        && breakout.close > zone.high + breakout_distance;
    const bool short_breakout = parameters.enable_short
        && breakout.close < breakout.open
        && breakout.close < zone.low - breakout_distance;
    if (!long_breakout && !short_breakout) {
        return std::nullopt;
    }

    const double tolerance = parameters.retest_tolerance_atr * atr;
    const double structural_buffer = parameters.stop_buffer_atr * atr;
    const auto current_spread = bars[confirmation_index].bid_ask_spread.value_or(0.0);
    const double half_spread = current_spread * 0.5;

    if (long_breakout) {
        const double stop = zone.low - structural_buffer - half_spread;
        bool retest_seen = false;
        bool earlier_confirmation = false;
        for (std::size_t index = breakout_index + 1; index <= confirmation_index; ++index) {
            if (bars[index].close <= zone.low - structural_buffer) {
                return std::nullopt;
            }
            retest_seen = retest_seen || long_retest_touch(bars[index], zone, tolerance);
            if (index < confirmation_index && retest_seen
                && long_confirmation(bars, index, zone)) {
                earlier_confirmation = true;
            }
        }
        if (retest_seen && !earlier_confirmation
            && long_confirmation(bars, confirmation_index, zone)
            && stop > 0.0) {
            const double signal_exit_quote = bars[confirmation_index].close - half_spread;
            const double risk = signal_exit_quote - stop;
            if (!(risk > 0.0)) {
                return std::nullopt;
            }
            return TradeSignal{
                PositionSide::long_position,
                stop,
                parameters.use_target
                    ? std::optional<double>{signal_exit_quote + parameters.target_r * risk}
                    : std::nullopt,
                "spartak_price_baseline_v0_1.long"};
        }
    }

    if (short_breakout) {
        const double stop = zone.high + structural_buffer + half_spread;
        bool retest_seen = false;
        bool earlier_confirmation = false;
        for (std::size_t index = breakout_index + 1; index <= confirmation_index; ++index) {
            if (bars[index].close >= zone.high + structural_buffer) {
                return std::nullopt;
            }
            retest_seen = retest_seen || short_retest_touch(bars[index], zone, tolerance);
            if (index < confirmation_index && retest_seen
                && short_confirmation(bars, index, zone)) {
                earlier_confirmation = true;
            }
        }
        if (retest_seen && !earlier_confirmation
            && short_confirmation(bars, confirmation_index, zone)) {
            const double signal_exit_quote = bars[confirmation_index].close + half_spread;
            const double risk = stop - signal_exit_quote;
            if (!(risk > 0.0)) {
                return std::nullopt;
            }
            return TradeSignal{
                PositionSide::short_position,
                stop,
                parameters.use_target
                    ? std::optional<double>{signal_exit_quote - parameters.target_r * risk}
                    : std::nullopt,
                "spartak_price_baseline_v0_1.short"};
        }
    }
    return std::nullopt;
}

}  // namespace

std::optional<TradeSignal> SpartakPriceBaselineStrategy::on_bar_close(
    const std::span<const MarketBar> bars,
    const std::size_t bar_index,
    const StrategyParameters& parameters) const {
    const BaselineParameters parsed = load_parameters(parameters);
    if (bar_index >= bars.size() || bars.size() != bar_index + 1) {
        throw std::invalid_argument(
            "baseline strategy requires a causal span ending at bar_index");
    }
    for (const auto& bar : bars) {
        bar.validate();
    }

    const std::size_t minimum_history = std::max(
        parsed.atr_period,
        parsed.accumulation_bars);
    if (bar_index < minimum_history + 1) {
        return std::nullopt;
    }

    const std::size_t maximum_age = std::min(parsed.retest_max_bars, bar_index);
    for (std::size_t age = 1; age <= maximum_age; ++age) {
        const std::size_t breakout_index = bar_index - age;
        if (breakout_index < parsed.accumulation_bars) {
            break;
        }
        if (auto signal = evaluate_candidate(
                bars,
                bar_index,
                breakout_index,
                parsed)) {
            return signal;
        }
    }
    return std::nullopt;
}

}  // namespace spartak
