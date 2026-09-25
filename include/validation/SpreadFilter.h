// =============================================================================
//  SPARTAK :: validation/SpreadFilter.h
//  Фильтр спреда: пропускает сигнал, только если спред в пределах лимита.
//
//  Использование в SignalValidator:
//    if (!spread_filter.pass(bar.spread)) reject(SpreadTooHigh);
//
//  Два порога:
//    max_points       — абсолютный максимум (в пунктах);
//    max_avg_ratio    — относительно среднего спреда (0 = выключен).
// =============================================================================
#pragma once
#include "core/Types.h"
#include "core/Constants.h"
#include <cstdint>
#include <vector>
namespace spartak::validation {
// -----------------------------------------------------------------------------
// Конфиг.
// -----------------------------------------------------------------------------
struct SpreadFilterConfig {
    int32_t max_points      = 25;     // абсолютный лимит в пунктах
    double  max_avg_ratio   = 0.0;    // относительный лимит (0 = выключен)
    int     avg_period      = 50;     // окно среднего
};
// -----------------------------------------------------------------------------
// SpreadFilter — stateless.
// -----------------------------------------------------------------------------
class SpreadFilter {
public:
    explicit SpreadFilter(SpreadFilterConfig cfg = {});
    // Проверка одного спреда.
    [[nodiscard]] bool pass(int32_t spread_points) const noexcept;
    // Проверка спреда относительно среднего по истории.
    [[nodiscard]] bool passWithAverage(int32_t spread_points,
                                       const std::vector<core::Bar>& history) const noexcept;
    // Комбинированная проверка (оба критерия).
    [[nodiscard]] bool passAll(int32_t spread_points,
                               const std::vector<core::Bar>& history) const noexcept;
    // Средний спред по последним period барам (0 при недостатке данных).
    [[nodiscard]] double averageSpread(const std::vector<core::Bar>& history,
                                       int period = -1) const noexcept;
    const SpreadFilterConfig& config() const noexcept { return cfg_; }
private:
    SpreadFilterConfig cfg_;
};
} // namespace spartak::validation