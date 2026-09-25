#include "patterns/InsideBarDetector.h"
#include <algorithm>
#include <stdexcept>
namespace spartak::patterns {
InsideBarDetector::InsideBarDetector(InsideBarConfig cfg)
    : cfg_(cfg) {
    if (cfg_.max_range_ratio <= 0.0)
        throw std::invalid_argument("InsideBarConfig::max_range_ratio must be > 0");
    if (cfg_.min_range_ratio < 0.0)
        throw std::invalid_argument("InsideBarConfig::min_range_ratio must be >= 0");
    if (cfg_.min_range_ratio > cfg_.max_range_ratio)
        throw std::invalid_argument("min_range_ratio must be <= max_range_ratio");
}
InsideBarSignal InsideBarDetector::detect(const core::Bar& prev,
                                          const core::Bar& curr) const noexcept
{
    InsideBarSignal sig;
    const double mother_range = prev.high - prev.low;
    const double inside_range = curr.high - curr.low;
    if (mother_range <= 0.0) return sig;
    // Внутренний бар: high ≤ mother.high И low ≥ mother.low
    const bool inside = (curr.high <= prev.high) && (curr.low >= prev.low);
    if (!inside) return sig;
    const double ratio = inside_range / mother_range;
    if (ratio > cfg_.max_range_ratio) return sig;
    if (ratio < cfg_.min_range_ratio) return sig;
    sig.detected    = true;
    sig.mother_high = prev.high;
    sig.mother_low  = prev.low;
    sig.range_ratio = ratio;
    // confidence: чем сильнее сжатие (ratio ближе к 0), тем выше уверенность.
    // Формула: 1 - ratio (нормировано на [0,1]).
    sig.confidence = std::max(0.0, std::min(1.0, 1.0 - ratio));
    return sig;
}
InsideBarSignal InsideBarDetector::detectLast(
        const std::vector<core::Bar>& bars) const noexcept
{
    if (bars.size() < 2) return {};
    return detect(bars[bars.size() - 2], bars[bars.size() - 1]);
}
} // namespace spartak::patterns