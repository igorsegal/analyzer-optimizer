// =============================================================================
//  SPARTAK :: context/SourceZoneDetector.cpp
// =============================================================================
#include "context/SourceZoneDetector.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace spartak::context {
SourceZoneDetector::SourceZoneDetector(SourceZoneConfig cfg)
    : cfg_(cfg)
{
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("SourceZoneConfig::point must be > 0");
    if (cfg_.min_bars_in_zone < 3)
        throw std::invalid_argument("SourceZoneConfig::min_bars_in_zone must be >= 3");
    if (cfg_.max_bars_in_zone < cfg_.min_bars_in_zone)
        throw std::invalid_argument("max_bars_in_zone < min_bars_in_zone");
    if (cfg_.max_range_points <= 0)
        throw std::invalid_argument("max_range_points must be > 0");
    if (cfg_.breakout_points <= 0)
        throw std::invalid_argument("breakout_points must be > 0");
    if (cfg_.confirm_bars < 1)
        throw std::invalid_argument("confirm_bars must be >= 1");
}
SourceZone SourceZoneDetector::find(const std::vector<core::Bar>& bars) const
{
    SourceZone best;
    if (bars.size() < cfg_.min_bars_in_zone + cfg_.confirm_bars + 2)
        return best;
    const double max_range  = static_cast<double>(cfg_.max_range_points) * cfg_.point;
    const double breakout   = static_cast<double>(cfg_.breakout_points)  * cfg_.point;
    const std::size_t N = bars.size();
    // Сканируем окна от конца к началу — ищем самый свежий источник.
    for (std::size_t z_end = N - cfg_.confirm_bars - 1; z_end >= cfg_.min_bars_in_zone; --z_end)
    {
        // Для каждой длины окна от min до max
        for (std::size_t len = cfg_.min_bars_in_zone; len <= cfg_.max_bars_in_zone; ++len)
        {
            if (z_end < len) break;
            const std::size_t z_start = z_end - len;
            // Границы зоны
            double z_high = bars[z_start].high;
            double z_low  = bars[z_start].low;
            for (std::size_t i = z_start + 1; i < z_end; ++i) {
                if (bars[i].high > z_high) z_high = bars[i].high;
                if (bars[i].low  < z_low)  z_low  = bars[i].low;
            }
            if ((z_high - z_low) > max_range) continue;
            // Ищем выход за границы после конца зоны, до N - confirm_bars
            for (std::size_t i = z_end; i < N - cfg_.confirm_bars; ++i)
            {
                const double up_level   = z_high + breakout;
                const double down_level = z_low  - breakout;
                bool broke_up   = (bars[i].high >= up_level);
                bool broke_down = (bars[i].low  <= down_level);
                if (!broke_up && !broke_down) continue;
                const core::TrendDirection dir = broke_up
                    ? core::TrendDirection::Bullish
                    : core::TrendDirection::Bearish;
                // Проверяем: не вернулась ли цена в зону за confirm_bars
                bool returned = false;
                const std::size_t until = std::min(i + cfg_.confirm_bars, N);
                for (std::size_t j = i + 1; j < until; ++j) {
                    if (dir == core::TrendDirection::Bullish &&
                        bars[j].low <= z_high) { returned = true; break; }
                    if (dir == core::TrendDirection::Bearish &&
                        bars[j].high >= z_low) { returned = true; break; }
                }
                if (returned) continue;
                // Нашли валидный источник
                SourceZone s;
                s.found      = true;
                s.top        = z_high;
                s.bottom     = z_low;
                s.level      = (dir == core::TrendDirection::Bullish) ? z_high : z_low;
                s.start_index = z_start;
                s.end_index   = z_end;
                s.direction   = dir;
                return s;
            }
        }
    }
    return best;
}
} // namespace spartak::context