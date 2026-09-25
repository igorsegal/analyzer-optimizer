// =============================================================================
//  SPARTAK :: patterns/InsideBarDetector.h
//  Детектор Inside Bar.
//
//  Inside Bar — свеча, полностью находящаяся внутри диапазона предыдущей:
//    bar[i].high <= bar[i-1].high
//    bar[i].low  >= bar[i-1].low
//
//  В методике ТАП Inside Bar — сигнал сжатия перед импульсом.
//  Сторона будущего пробоя (up/down) определяется отдельно
//  (например, направлением mother bar).
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstddef>
#include <vector>
namespace spartak::patterns {
// -----------------------------------------------------------------------------
// Результат детекции.
// -----------------------------------------------------------------------------
struct InsideBarSignal {
    bool   detected       = false;
    double confidence     = 0.0;    // 0..1
    double mother_high    = 0.0;    // high материнской свечи
    double mother_low     = 0.0;    // low материнской свечи
    double range_ratio    = 0.0;    // inside_range / mother_range
};
// -----------------------------------------------------------------------------
// Конфиг.
// -----------------------------------------------------------------------------
struct InsideBarConfig {
    double max_range_ratio = 1.0;   // внутри-свеча должна быть не шире материнской
    double min_range_ratio = 0.0;   // (опционально) минимальная «сжатость»
};
// -----------------------------------------------------------------------------
// InsideBarDetector — stateless.
// -----------------------------------------------------------------------------
class InsideBarDetector {
public:
    explicit InsideBarDetector(InsideBarConfig cfg = {});
    // Проверка двух последовательных баров:
    //   prev — материнская свеча (bar[i-1])
    //   curr — проверяемая свеча (bar[i])
    [[nodiscard]] InsideBarSignal detect(const core::Bar& prev,
                                         const core::Bar& curr) const noexcept;
    // Утилита: проверить последний бар в истории как curr.
    [[nodiscard]] InsideBarSignal detectLast(const std::vector<core::Bar>& bars) const noexcept;
    const InsideBarConfig& config() const noexcept { return cfg_; }
private:
    InsideBarConfig cfg_;
};
} // namespace spartak::patterns