#include "patterns/CandleGeometryCalculator.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace spartak::patterns {
// -----------------------------------------------------------------------------
// Конструктор с валидацией.
// -----------------------------------------------------------------------------
CandleGeometryCalculator::CandleGeometryCalculator(CandleGeometryConfig cfg)
    : cfg_(cfg) {
    if (cfg_.doji_body_ratio < 0.0 || cfg_.doji_body_ratio > 1.0)
        throw std::invalid_argument("CandleGeometryConfig::doji_body_ratio must be in [0,1]");
    if (cfg_.atr_period < 1)
        throw std::invalid_argument("CandleGeometryConfig::atr_period must be >= 1");
}
// -----------------------------------------------------------------------------
// compute — геометрия одной свечи.
//
// Защита: если range <= 0 (битый бар) — все отношения возвращаются как 0.
// -----------------------------------------------------------------------------
CandleGeometry CandleGeometryCalculator::compute(const core::Bar& bar) const noexcept {
    CandleGeometry g;
    g.range = bar.high - bar.low;
    if (g.range <= 0.0) {
        return g;   // всё остальное остаётся нулями
    }
    g.body       = std::fabs(bar.close - bar.open);
    g.upper_wick = bar.high - std::max(bar.open, bar.close);
    g.lower_wick = std::min(bar.open, bar.close) - bar.low;
    g.body_ratio     = g.body       / g.range;
    g.upper_ratio    = g.upper_wick / g.range;
    g.lower_ratio    = g.lower_wick / g.range;
    g.close_position = (bar.close - bar.low) / g.range;
    g.is_bull = (bar.close > bar.open);
    g.is_bear = (bar.close < bar.open);
    g.is_doji = (g.body_ratio < cfg_.doji_body_ratio);
    // Защита от отрицательных значений из-за floating-point
    g.body_ratio     = std::max(0.0, g.body_ratio);
    g.upper_ratio    = std::max(0.0, g.upper_ratio);
    g.lower_ratio    = std::max(0.0, g.lower_ratio);
    g.close_position = std::max(0.0, std::min(1.0, g.close_position));
    return g;
}
// -----------------------------------------------------------------------------
// atr — Average True Range.
//
// TR(i) = max(
//     high[i] - low[i],
//     |high[i] - close[i-1]|,
//     |low[i]  - close[i-1]|
// )
// ATR = среднее TR по последним period барам (простое скользящее).
//
// Если баров меньше period+1 — возвращаем 0.
// Если period < 0 — берём cfg_.atr_period.
// -----------------------------------------------------------------------------
double CandleGeometryCalculator::atr(const std::vector<core::Bar>& bars,
                                     int period) const noexcept {
    const int p = (period < 0) ? cfg_.atr_period : period;
    if (p < 1) return 0.0;
    if (bars.size() < static_cast<std::size_t>(p + 1)) return 0.0;
    const std::size_t n = bars.size();
    const std::size_t start = n - static_cast<std::size_t>(p);
    double sum = 0.0;
    for (std::size_t i = start; i < n; ++i) {
        const double hl = bars[i].high - bars[i].low;
        const double hc = std::fabs(bars[i].high - bars[i - 1].close);
        const double lc = std::fabs(bars[i].low  - bars[i - 1].close);
        const double tr = std::max({hl, hc, lc});
        sum += tr;
    }
    return sum / static_cast<double>(p);
}
// -----------------------------------------------------------------------------
// average_range — средний high-low по последним period барам.
// -----------------------------------------------------------------------------
double CandleGeometryCalculator::average_range(const std::vector<core::Bar>& bars,
                                               int period) const noexcept {
    const int p = (period < 0) ? cfg_.atr_period : period;
    if (p < 1) return 0.0;
    if (bars.size() < static_cast<std::size_t>(p)) return 0.0;
    const std::size_t n = bars.size();
    const std::size_t start = n - static_cast<std::size_t>(p);
    double sum = 0.0;
    for (std::size_t i = start; i < n; ++i) {
        sum += (bars[i].high - bars[i].low);
    }
    return sum / static_cast<double>(p);
}
} // namespace spartak::patterns