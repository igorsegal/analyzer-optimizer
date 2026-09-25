// =============================================================================
//  SPARTAK :: position/BreakEvenTransfer.h
//  Расчёт нового стоп-лосса при переносе в безубыток (BE).
//
//  Логика:
//    После срабатывания TP1 стоп переносится в зону входа + компенсация спреда,
//    чтобы позиция стала «безрисковой» по факту закрытия.
//
//    BUY:  new_sl = entry + spread_points * point + buffer_points * point
//    SELL: new_sl = entry - spread_points * point - buffer_points * point
//
//  Дополнительно: BE-clamp — не должен перепрыгивать TP1, иначе позиция
//  закроется мгновенно на следующем баре. Если new_sl перепрыгивает TP1,
//  сдвигаем на clamp_buffer_points ПЕРЕД TP1.
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstdint>
namespace spartak::position {
// -----------------------------------------------------------------------------
// Конфиг.
// -----------------------------------------------------------------------------
struct BreakEvenConfig {
    bool    use_spread_in_be       = true;
    int32_t fallback_spread_points = 12;   // если у позиции не задан спред
    int     extra_buffer_points    = 0;    // дополнительный отступ
    int     clamp_buffer_points    = 1;    // зазор от TP1 при клампе
    double  point                  = 0.00001;
};
// -----------------------------------------------------------------------------
// Результат.
// -----------------------------------------------------------------------------
struct BreakEvenResult {
    bool   ok       = false;
    bool   clamped  = false;   // сработал clamp (близко к TP1)
    double new_sl   = 0.0;
    double move_pts = 0.0;     // на сколько пунктов сдвинули стоп (диагностика)
};
// -----------------------------------------------------------------------------
// BreakEvenTransfer — stateless.
// -----------------------------------------------------------------------------
class BreakEvenTransfer {
public:
    explicit BreakEvenTransfer(BreakEvenConfig cfg = {});
    // compute(side, entry, tp1, spread_pts, current_sl)
    //   side         — сторона позиции
    //   entry        — цена входа
    //   tp1          — первая цель (для clamp)
    //   spread_pts   — спред на момент входа (в пунктах)
    //   current_sl   — текущий стоп (для расчёта move_pts)
    [[nodiscard]] BreakEvenResult compute(core::OrderSide side,
                                          double entry,
                                          double tp1,
                                          int32_t spread_pts,
                                          double current_sl) const noexcept;
    const BreakEvenConfig& config() const noexcept { return cfg_; }
private:
    BreakEvenConfig cfg_;
};
} // namespace spartak::position