// =============================================================================
//  SPARTAK :: position/UO1Trigger.h
//  Триггер первой цели (UO1 = Unit Of 1st target).
//
//  Определяет, что произошло на текущем баре относительно позиции:
//    - SL_Hit   — цена пробила стоп-лосс;
//    - TP1_Hit  — цена достигла первой цели;
//    - TP2_Hit  — цена достигла основной цели;
//    - None     — ничего.
//
//  Проверка по high/low бара (worst-case: внутри бара могли быть оба уровня).
//  Приоритет: SL > TP2 > TP1 (риск важнее прибыли).
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstdint>
namespace spartak::position {
// -----------------------------------------------------------------------------
// Событие, зафиксированное на баре.
// -----------------------------------------------------------------------------
enum class TriggerEvent {
    None      = 0,
    SL_Hit    = 1,
    TP1_Hit   = 2,
    TP2_Hit   = 3
};
[[nodiscard]] const char* to_string(TriggerEvent e) noexcept;
// -----------------------------------------------------------------------------
// Параметры позиции, необходимые для проверки.
// -----------------------------------------------------------------------------
struct TriggerContext {
    core::OrderSide side    = core::OrderSide::Buy;
    double          sl      = 0.0;
    double          tp1     = 0.0;
    double          tp2     = 0.0;
    bool            tp1_hit = false;   // TP1 уже отработал (для пропуска)
    bool            tp2_hit = false;
};
// -----------------------------------------------------------------------------
// UO1Trigger — stateless.
// -----------------------------------------------------------------------------
class UO1Trigger {
public:
    UO1Trigger() = default;
    // Проверка одной свечи.
    [[nodiscard]] TriggerEvent check(const core::Bar& bar,
                                     const TriggerContext& pos) const noexcept;
    // Проверка без учёта tp1_hit/tp2_hit (всегда проверяет оба уровня).
    [[nodiscard]] TriggerEvent checkRaw(const core::Bar& bar,
                                        core::OrderSide side,
                                        double sl,
                                        double tp1,
                                        double tp2) const noexcept;
};
} // namespace spartak::position