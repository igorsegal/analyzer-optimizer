#include "patterns/TickVolumeFilter.h"
#include <algorithm>
#include <stdexcept>
namespace spartak::patterns {
// -----------------------------------------------------------------------------
// Конструктор с валидацией.
// -----------------------------------------------------------------------------
TickVolumeFilter::TickVolumeFilter(TickVolumeConfig cfg)
    : cfg_(cfg) {
    if (cfg_.min_volume < 0)
        throw std::invalid_argument("TickVolumeConfig::min_volume must be >= 0");
    if (cfg_.avg_ratio < 0.0)
        throw std::invalid_argument("TickVolumeConfig::avg_ratio must be >= 0");
    if (cfg_.avg_period < 1)
        throw std::invalid_argument("TickVolumeConfig::avg_period must be >= 1");
}
// -----------------------------------------------------------------------------
// pass — абсолютная проверка.
// Если min_volume == 0 — фильтр выключен, пропускаем всё.
// -----------------------------------------------------------------------------
bool TickVolumeFilter::pass(const core::Bar& bar) const noexcept {
    if (cfg_.min_volume <= 0) return true;
    return bar.tick_volume >= cfg_.min_volume;
}
// -----------------------------------------------------------------------------
// averageVolume — средний объём за последние period баров.
// -----------------------------------------------------------------------------
double TickVolumeFilter::averageVolume(const std::vector<core::Bar>& bars,
                                       int period) const noexcept {
    const int p = (period < 0) ? cfg_.avg_period : period;
    if (p < 1) return 0.0;
    if (bars.size() < static_cast<std::size_t>(p)) return 0.0;
    const std::size_t n = bars.size();
    const std::size_t start = n - static_cast<std::size_t>(p);
    double sum = 0.0;
    for (std::size_t i = start; i < n; ++i) {
        sum += static_cast<double>(bars[i].tick_volume);
    }
    return sum / static_cast<double>(p);
}
// -----------------------------------------------------------------------------
// passAverage — относительная проверка.
//
// Смотрим среднее по последним avg_period барам (включая текущий).
// Текущий бар должен иметь объём >= avg_ratio * среднее.
//
// Если данных меньше avg_period — фильтр не может оценить среднее,
// пропускаем (не блокируем стратегию на старте).
// -----------------------------------------------------------------------------
bool TickVolumeFilter::passAverage(const std::vector<core::Bar>& bars) const noexcept {
    if (bars.empty()) return false;
    if (bars.size() < static_cast<std::size_t>(cfg_.avg_period)) return true;
    const double avg = averageVolume(bars, cfg_.avg_period);
    if (avg <= 0.0) return true;   // пустые объёмы — не блокируем
    const double threshold = cfg_.avg_ratio * avg;
    return static_cast<double>(bars.back().tick_volume) >= threshold;
}
// -----------------------------------------------------------------------------
// passAll — оба критерия.
// -----------------------------------------------------------------------------
bool TickVolumeFilter::passAll(const core::Bar& bar,
                               const std::vector<core::Bar>& bars) const noexcept {
    if (!pass(bar)) return false;
    if (!passAverage(bars)) return false;
    return true;
}
} // namespace spartak::patterns