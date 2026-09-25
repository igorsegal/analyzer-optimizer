// =============================================================================
//  SPARTAK :: position/EmergencyCloseHandler.h
//  Аварийное закрытие позиции.
//
//  Триггеры:
//    1. MarginCall — уровень маржи провалился ниже критического порога
//    2. Drawdown — просадка по позиции превысила допустимый лимит
//    3. MaxHoldingDays — позиция висит дольше N дней (swap съест прибыль)
//
//  Модуль решает: должна ли позиция быть закрыта немедленно,
//  и возвращает причину закрытия для отчётности.
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstdint>
namespace spartak::position {
// -----------------------------------------------------------------------------
// Причина аварийного закрытия.
// -----------------------------------------------------------------------------
enum class EmergencyReason {
    None = 0,
    MarginCall,
    DrawdownLimit,
    MaxHoldingTime
};
[[nodiscard]] const char* to_string(EmergencyReason r) noexcept;
// -----------------------------------------------------------------------------
// Конфиг.
// -----------------------------------------------------------------------------
struct EmergencyConfig {
    double  min_margin_level_pct     = 1000.0;   // ниже этого -> margin call
    double  max_drawdown_pct         = 50.0;     // % от risk_money на позицию
    int     max_holding_days         = 30;       // дольше -> принудительное закрытие
    bool    margin_check_enabled     = true;
    bool    drawdown_check_enabled   = true;
    bool    holding_check_enabled    = true;
};
// -----------------------------------------------------------------------------
// Вход.
// -----------------------------------------------------------------------------
struct EmergencyContext {
    double  margin_level_pct  = 0.0;   // текущий MarginLevel (%)
    double  risk_money        = 0.0;   // риск в USD на позицию (для drawdown)
    double  current_pnl       = 0.0;   // текущий PnL в USD (может быть отрицательным)
    int64_t open_time_ms      = 0;
    int64_t now_ms            = 0;
};
// -----------------------------------------------------------------------------
// Результат.
// -----------------------------------------------------------------------------
struct EmergencyDecision {
    bool             should_close = false;
    EmergencyReason  reason       = EmergencyReason::None;
    double           value        = 0.0;   // для диагностики: margin%, dd%, days
};
// -----------------------------------------------------------------------------
// EmergencyCloseHandler — stateless.
// -----------------------------------------------------------------------------
class EmergencyCloseHandler {
public:
    explicit EmergencyCloseHandler(EmergencyConfig cfg = {});
    [[nodiscard]] EmergencyDecision check(const EmergencyContext& ctx) const noexcept;
    const EmergencyConfig& config() const noexcept { return cfg_; }
private:
    EmergencyConfig cfg_;
};
} // namespace spartak::position