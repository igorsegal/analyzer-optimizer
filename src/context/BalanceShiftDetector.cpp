// =============================================================================
//  SPARTAK :: context/BalanceShiftDetector.cpp
// =============================================================================
#include "context/BalanceShiftDetector.h"
#include "context/SourceZoneDetector.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace spartak::context {
BalanceShiftDetector::BalanceShiftDetector(BalanceShiftConfig cfg)
    : cfg_(cfg)
{
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("BalanceShiftConfig::point must be > 0");
    if (cfg_.min_gap_bars >= cfg_.max_gap_bars)
        throw std::invalid_argument("min_gap_bars must be < max_gap_bars");
}
BalanceShift BalanceShiftDetector::detect(
    const std::vector<SourceZone>& zones) const
{
    BalanceShift out;
    if (zones.size() < 2) return out;
    // Берём две последние зоны
    const SourceZone& first  = zones[zones.size() - 2];
    const SourceZone& second = zones[zones.size() - 1];
    if (!first.found || !second.found) return out;
    if (second.end_index <= first.end_index) return out;
    const std::size_t gap = second.start_index > first.end_index
        ? (second.start_index - first.end_index)
        : 0;
    if (gap < cfg_.min_gap_bars) return out;
    if (gap > cfg_.max_gap_bars) return out;
    out.found            = true;
    out.first_zone_end   = first.end_index;
    out.second_zone_end  = second.end_index;
    // Направление перевеса = направление последней зоны
    if (second.direction == core::TrendDirection::Bullish) {
        out.side = BalanceSide::Bullish;
    } else if (second.direction == core::TrendDirection::Bearish) {
        out.side = BalanceSide::Bearish;
    } else {
        out.found = false;
        return out;
    }
    // Сила: чем меньше gap — тем сильнее перевес
    const double g = static_cast<double>(gap);
    const double gmax = static_cast<double>(cfg_.max_gap_bars);
    out.strength = std::max(0.0, std::min(1.0, 1.0 - g / gmax));
    return out;
}
} // namespace spartak::context