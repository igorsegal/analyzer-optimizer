#include "position/UO1Trigger.h"
namespace spartak::position {
const char* to_string(TriggerEvent e) noexcept {
    switch (e) {
        case TriggerEvent::None:    return "None";
        case TriggerEvent::SL_Hit:  return "SL_Hit";
        case TriggerEvent::TP1_Hit: return "TP1_Hit";
        case TriggerEvent::TP2_Hit: return "TP2_Hit";
    }
    return "Unknown";
}
// -----------------------------------------------------------------------------
// checkRaw — проверка по high/low свечи, без учёта уже сработавших уровней.
//
// Приоритет: SL > TP2 > TP1.
// Это защищает от ситуации, когда внутри одной свечи цена успела сходить
// и в стоп, и в цель — считаем «сработал стоп», консервативно.
// -----------------------------------------------------------------------------
TriggerEvent UO1Trigger::checkRaw(const core::Bar& bar,
                                  core::OrderSide side,
                                  double sl,
                                  double tp1,
                                  double tp2) const noexcept
{
    if (side == core::OrderSide::Buy) {
        // SL ниже цены, TP выше
        if (sl > 0.0 && bar.low <= sl) return TriggerEvent::SL_Hit;
        if (tp2 > 0.0 && bar.high >= tp2) return TriggerEvent::TP2_Hit;
        if (tp1 > 0.0 && bar.high >= tp1) return TriggerEvent::TP1_Hit;
    } else {
        // SELL: SL выше цены, TP ниже
        if (sl > 0.0 && bar.high >= sl) return TriggerEvent::SL_Hit;
        if (tp2 > 0.0 && bar.low <= tp2) return TriggerEvent::TP2_Hit;
        if (tp1 > 0.0 && bar.low <= tp1) return TriggerEvent::TP1_Hit;
    }
    return TriggerEvent::None;
}
// -----------------------------------------------------------------------------
// check — с учётом того, что TP1/TP2 уже могли отработать.
// -----------------------------------------------------------------------------
TriggerEvent UO1Trigger::check(const core::Bar& bar,
                               const TriggerContext& pos) const noexcept
{
    if (pos.side == core::OrderSide::Buy) {
        if (pos.sl > 0.0 && bar.low <= pos.sl) return TriggerEvent::SL_Hit;
        if (!pos.tp2_hit && pos.tp2 > 0.0 && bar.high >= pos.tp2)
            return TriggerEvent::TP2_Hit;
        if (!pos.tp1_hit && pos.tp1 > 0.0 && bar.high >= pos.tp1)
            return TriggerEvent::TP1_Hit;
    } else {
        if (pos.sl > 0.0 && bar.high >= pos.sl) return TriggerEvent::SL_Hit;
        if (!pos.tp2_hit && pos.tp2 > 0.0 && bar.low <= pos.tp2)
            return TriggerEvent::TP2_Hit;
        if (!pos.tp1_hit && pos.tp1 > 0.0 && bar.low <= pos.tp1)
            return TriggerEvent::TP1_Hit;
    }
    return TriggerEvent::None;
}
} // namespace spartak::position