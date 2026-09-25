#include "validation/CounterTrendPUChecker.h"
#include <cmath>
#include <stdexcept>
namespace spartak::validation {
CounterTrendPUChecker::CounterTrendPUChecker(CounterTrendConfig cfg)
    : cfg_(cfg) {
    if (cfg_.pu_match_epsilon_points < 0.0)
        throw std::invalid_argument("CounterTrendConfig::pu_match_epsilon_points must be >= 0");
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("CounterTrendConfig::point must be > 0");
}
bool CounterTrendPUChecker::level_on_pu(double level,
                                       const core::MarketContext& ctx) const noexcept {
    const double eps = cfg_.pu_match_epsilon_points * cfg_.point;
    for (const auto& z : ctx.active_zones) {
        if (!z.is_active) continue;
        if (z.type != core::LevelType::IntermediateLevel) continue;
        if (std::fabs(z.price_level - level) <= eps) return true;
        if (level >= z.zone_bottom && level <= z.zone_top) return true;
    }
    return false;
}
CounterTrendResult CounterTrendPUChecker::check(
        core::OrderSide               signal_side,
        core::TrendDirection          trend,
        double                        level,
        const core::MarketContext&    ctx) const noexcept
{
    CounterTrendResult r;
    if (trend == core::TrendDirection::Undefined) {
        r.allowed    = true;
        r.is_counter = false;
        return r;
    }
    const bool buy_with_bull  = (signal_side == core::OrderSide::Buy  &&
                                 trend == core::TrendDirection::Bullish);
    const bool sell_with_bear = (signal_side == core::OrderSide::Sell &&
                                 trend == core::TrendDirection::Bearish);
    if (buy_with_bull || sell_with_bear) {
        r.allowed    = true;
        r.is_counter = false;
        return r;
    }
    r.is_counter = true;
    if (!cfg_.allow_counter_trend_on_pu) {
        r.allowed = false;
        return r;
    }
    if (level_on_pu(level, ctx)) {
        r.allowed   = true;
        r.pu_excuse = true;
        return r;
    }
    r.allowed = false;
    return r;
}
} // namespace spartak::validation