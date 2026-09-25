// =============================================================================
//  SPARTAK :: patterns/TickVolumeFilter.h
//  Фильтр по тиковому объёму свечи.
//
//  Используется в детекторах паттернов: сигналы допускаются только
//  от «значимых» свечей — с активностью выше порога.
//
//  Два режима:
//    1. Абсолютный:   bar.tick_volume >= min_volume
//    2. Относительный: bar.tick_volume >= ratio * avg_volume(period)
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstddef>
#include <vector>
namespace spartak::patterns {
// -----------------------------------------------------------------------------
// Конфиг фильтра.
// -----------------------------------------------------------------------------
struct TickVolumeConfig {
    int64_t min_volume      = 0;     // абсолютный минимум (0 = без фильтра)
    double  avg_ratio       = 0.8;   // доля от среднего (0.8 = 80%)
    int     avg_period      = 20;    // окно среднего
};
// -----------------------------------------------------------------------------
// TickVolumeFilter — stateless.
// -----------------------------------------------------------------------------
class TickVolumeFilter {
public:
    explicit TickVolumeFilter(TickVolumeConfig cfg = {});
    // Абсолютная проверка.
    [[nodiscard]] bool pass(const core::Bar& bar) const noexcept;
    // Относительная: bar.tick_volume >= avg_ratio * mean(last period bars).
    // bars — история, включая сам bar в конце.
    [[nodiscard]] bool passAverage(const std::vector<core::Bar>& bars) const noexcept;
    // Комбинированная: оба критерия должны пройти.
    [[nodiscard]] bool passAll(const core::Bar& bar,
                               const std::vector<core::Bar>& bars) const noexcept;
    // Средний объём за последние period баров (0 при недостатке данных).
    [[nodiscard]] double averageVolume(const std::vector<core::Bar>& bars,
                                       int period = -1) const noexcept;
    const TickVolumeConfig& config() const noexcept { return cfg_; }
private:
    TickVolumeConfig cfg_;
};
} // namespace spartak::patterns