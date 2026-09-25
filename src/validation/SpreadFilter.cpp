#include "validation/SpreadFilter.h"
#include <algorithm>
#include <stdexcept>
namespace spartak::validation {
SpreadFilter::SpreadFilter(SpreadFilterConfig cfg)
    : cfg_(cfg) {
    if (cfg_.max_points <= 0)
        throw std::invalid_argument("SpreadFilterConfig::max_points must be > 0");
    if (cfg_.max_avg_ratio < 0.0)
        throw std::invalid_argument("SpreadFilterConfig::max_avg_ratio must be >= 0");
    if (cfg_.avg_period < 1)
        throw std::invalid_argument("SpreadFilterConfig::avg_period must be >= 1");
}
// -----------------------------------------------------------------------------
// pass — абсолютная проверка.
// -----------------------------------------------------------------------------
bool SpreadFilter::pass(int32_t spread_points) const noexcept {
    return spread_points >= 0 && spread_points <= cfg_.max_points;
}
// -----------------------------------------------------------------------------
// averageSpread — средний спред за последние period баров.
// -----------------------------------------------------------------------------
double SpreadFilter::averageSpread(const std::vector<core::Bar>& history,
                                   int period) const noexcept {
    const int p = (period < 0) ? cfg_.avg_period : period;
    if (p < 1) return 0.0;
    if (history.size() < static_cast<std::size_t>(p)) return 0.0;
    const std::size_t n = history.size();
    const std::size_t start = n - static_cast<std::size_t>(p);
    double sum = 0.0;
    for (std::size_t i = start; i < n; ++i) {
        sum += static_cast<double>(history[i].spread);
    }
    return sum / static_cast<double>(p);
}
// -----------------------------------------------------------------------------
// passWithAverage — относительная проверка.
// Если max_avg_ratio = 0 — фильтр выключен (возвращает true).
// При недостатке данных — не блокируем.
// -----------------------------------------------------------------------------
bool SpreadFilter::passWithAverage(int32_t spread_points,
                                   const std::vector<core::Bar>& history) const noexcept {
    if (cfg_.max_avg_ratio <= 0.0) return true;
    if (history.size() < static_cast<std::size_t>(cfg_.avg_period)) return true;
    const double avg = averageSpread(history);
    if (avg <= 0.0) return true;
    const double threshold = cfg_.max_avg_ratio * avg;
    return static_cast<double>(spread_points) <= threshold;
}
// -----------------------------------------------------------------------------
// passAll — оба критерия.
// -----------------------------------------------------------------------------
bool SpreadFilter::passAll(int32_t spread_points,
                           const std::vector<core::Bar>& history) const noexcept {
    return pass(spread_points) && passWithAverage(spread_points, history);
}
} // namespace spartak::validation