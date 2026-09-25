#include "patterns/ATRImpulseBreakoutDetector.h"
#include <algorithm>
#include <stdexcept>
namespace spartak::patterns {
ATRImpulseBreakoutDetector::ATRImpulseBreakoutDetector(
        ImpulseConfig cfg, CandleGeometryConfig geom_cfg)
    : cfg_(cfg), geom_(geom_cfg) {
    if (cfg_.min_body_ratio < 0.0 || cfg_.min_body_ratio > 1.0)
        throw std::invalid_argument("ImpulseConfig::min_body_ratio must be in [0,1]");
    if (cfg_.min_close_pos < 0.0 || cfg_.min_close_pos > 1.0)
        throw std::invalid_argument("ImpulseConfig::min_close_pos must be in [0,1]");
    if (cfg_.atr_multiplier <= 0.0)
        throw std::invalid_argument("ImpulseConfig::atr_multiplier must be > 0");
    if (cfg_.atr_period < 1)
        throw std::invalid_argument("ImpulseConfig::atr_period must be >= 1");
    if (cfg_.stop_buffer_points < 0)
        throw std::invalid_argument("ImpulseConfig::stop_buffer_points must be >= 0");
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("ImpulseConfig::point must be > 0");
}
ImpulseSignal ATRImpulseBreakoutDetector::detect(
        const std::vector<core::Bar>& bars, double level) const noexcept
{
    ImpulseSignal sig;
    // Нужно минимум atr_period + 1 баров для ATR и сам бар
    if (bars.size() < static_cast<std::size_t>(cfg_.atr_period + 1)) return sig;
    const core::Bar& bar = bars.back();
    const auto g = geom_.compute(bar);
    if (g.range <= 0.0) return sig;
    const double atr = geom_.atr(bars, cfg_.atr_period);
    if (atr <= 0.0) return sig;
    sig.atr = atr;
    // Проверка размера тела
    if (g.body_ratio < cfg_.min_body_ratio) return sig;
    // Проверка размера свечи относительно ATR
    if (g.range < cfg_.atr_multiplier * atr) return sig;
    const double buffer = static_cast<double>(cfg_.stop_buffer_points) * cfg_.point;
    // ---------- BUY-импульс ----------
    if (g.is_bull && g.close_position >= cfg_.min_close_pos && bar.close > level) {
        sig.detected       = true;
        sig.is_bullish     = true;
        sig.trigger_price  = bar.close;
        sig.suggested_stop = bar.low - buffer;
        sig.level          = level;
        // confidence — комбинация трёх «превышений»:
        //   1. тело больше порога
        //   2. close ближе к high
        //   3. range больше ATR-порога
        const double c_body  = std::min(1.0,
            (g.body_ratio - cfg_.min_body_ratio) / (1.0 - cfg_.min_body_ratio));
        const double c_close = std::min(1.0,
            (g.close_position - cfg_.min_close_pos) / (1.0 - cfg_.min_close_pos));
        const double range_ratio = g.range / (cfg_.atr_multiplier * atr);
        const double c_range = std::min(1.0, range_ratio - 1.0);
        sig.confidence = (c_body + c_close + c_range) / 3.0;
        return sig;
    }
    // ---------- SELL-импульс ----------
    const double sell_close_pos = 1.0 - cfg_.min_close_pos;   // напр. 0.25
    if (g.is_bear && g.close_position <= sell_close_pos && bar.close < level) {
        sig.detected       = true;
        sig.is_bullish     = false;
        sig.trigger_price  = bar.close;
        sig.suggested_stop = bar.high + buffer;
        sig.level          = level;
        const double c_body  = std::min(1.0,
            (g.body_ratio - cfg_.min_body_ratio) / (1.0 - cfg_.min_body_ratio));
        const double c_close = std::min(1.0,
            (sell_close_pos - g.close_position) / sell_close_pos);
        const double range_ratio = g.range / (cfg_.atr_multiplier * atr);
        const double c_range = std::min(1.0, range_ratio - 1.0);
        sig.confidence = (c_body + c_close + c_range) / 3.0;
        return sig;
    }
    return sig;
}
} // namespace spartak::patterns