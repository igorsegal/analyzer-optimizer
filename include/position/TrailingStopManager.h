// =============================================================================
//  SPARTAK :: position/TrailingStopManager.h
//  Трейлинг стоп-лосса.
//
//  Логика (BUY):
//    new_sl = max(current_sl, bar.high - trailing_distance)
//    сдвигаем только если new_sl > current_sl
//
//  SELL:
//    new_sl = min(current_sl, bar.low + trailing_distance)
//    сдвигаем только если new_sl < current_sl
//
//  Activation (опционально):
//    трейлинг включается только после того, как цена прошла
//    activation_points в сторону прибыли.
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstdint>
namespace spartak::position {
// -----------------------------------------------------------------------------
// Конфиг.
// -----------------------------------------------------------------------------
struct TrailingConfig {
    int trailing_distance_points = 30;   // расстояние от текущей крайней точки
    int activation_points        = 0;    // 0 = трейлить сразу
    int min_step_points          = 1;    // минимальный шаг сдвига (антидребезг)
    double point                 = 0.00001;
};
// -----------------------------------------------------------------------------
// Результат.
// -----------------------------------------------------------------------------
struct TrailingResult {
    bool   moved    = false;
    double new_sl   = 0.0;
    double move_pts = 0.0;    // диагностика
};
// -----------------------------------------------------------------------------
// TrailingStopManager — stateless.
// -----------------------------------------------------------------------------
class TrailingStopManager {
public:
    explicit TrailingStopManager(TrailingConfig cfg = {});
    // compute(side, current_sl, entry, bar)
    [[nodiscard]] TrailingResult compute(core::OrderSide side,
                                         double current_sl,
                                         double entry,
                                         const core::Bar& bar) const noexcept;
    const TrailingConfig& config() const noexcept { return cfg_; }
private:
    TrailingConfig cfg_;
};
} // namespace spartak::position