#include "patterns/EngulfingDetector.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace spartak::patterns {
EngulfingDetector::EngulfingDetector(EngulfingConfig cfg)
    : cfg_(cfg) {
    if (cfg_.min_body_ratio_over_prev <= 0.0)
        throw std::invalid_argument("EngulfingConfig::min_body_ratio_over_prev must be > 0");
    if (cfg_.stop_buffer_points < 0)
        throw std::invalid_argument("EngulfingConfig::stop_buffer_points must be >= 0");
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("EngulfingConfig::point must be > 0");
}
EngulfingSignal EngulfingDetector::detect(const core::Bar& prev,
                                          const core::Bar& curr) const noexcept
{
    EngulfingSignal sig;
    const double prev_body = std::fabs(prev.close - prev.open);
    const double curr_body = std::fabs(curr.close - curr.open);
    if (prev_body <= 0.0 || curr_body <= 0.0) return sig;
    const bool prev_bull = prev.close > prev.open;
    const bool prev_bear = prev.close < prev.open;
    const bool curr_bull = curr.close > curr.open;
    const bool curr_bear = curr.close < curr.open;
    const double body_ratio = curr_body / prev_body;
    if (body_ratio < cfg_.min_body_ratio_over_prev) return sig;
    const double buffer = static_cast<double>(cfg_.stop_buffer_points) * cfg_.point;
    // ---------- Bullish Engulfing ----------
    if (prev_bear && curr_bull) {
        const bool engulfs_open  = (curr.open  <= prev.close);
        const bool engulfs_close = (curr.close >= prev.open);
        if (engulfs_open && engulfs_close) {
            sig.detected       = true;
            sig.is_bullish     = true;
            sig.trigger_price  = curr.close;
            sig.suggested_stop = curr.low - buffer;
            // confidence: 50% от "перекрытия" тела + 50% от размера
            const double overlap  = std::min(curr.close - prev.open,
                                             prev.close - curr.open);
            const double overlap_r = std::max(0.0, std::min(1.0, overlap / prev_body));
            const double size_r    = std::max(0.0, std::min(1.0, (body_ratio - 1.0)));
            sig.confidence = 0.5 * overlap_r + 0.5 * size_r;
            return sig;
        }
    }
    // ---------- Bearish Engulfing ----------
    if (prev_bull && curr_bear) {
        const bool engulfs_open  = (curr.open  >= prev.close);
        const bool engulfs_close = (curr.close <= prev.open);
        if (engulfs_open && engulfs_close) {
            sig.detected       = true;
            sig.is_bullish     = false;
            sig.trigger_price  = curr.close;
            sig.suggested_stop = curr.high + buffer;
            const double overlap  = std::min(prev.open - curr.close,
                                             curr.open - prev.close);
            const double overlap_r = std::max(0.0, std::min(1.0, overlap / prev_body));
            const double size_r    = std::max(0.0, std::min(1.0, (body_ratio - 1.0)));
            sig.confidence = 0.5 * overlap_r + 0.5 * size_r;
            return sig;
        }
    }
    return sig;
}
EngulfingSignal EngulfingDetector::detectLast(
        const std::vector<core::Bar>& bars) const noexcept
{
    if (bars.size() < 2) return {};
    return detect(bars[bars.size() - 2], bars[bars.size() - 1]);
}
} // namespace spartak::patterns