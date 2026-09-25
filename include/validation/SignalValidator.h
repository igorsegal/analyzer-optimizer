// =============================================================================
//  SPARTAK :: validation/SignalValidator.h
//  Финальный агрегатор слоя validation.
//
//  Прогоняет сигнал PatternSignal + контекст + счёт через все фильтры:
//    1. SpreadFilter          — спред в пределах
//    2. CounterTrendPUChecker — направление относительно тренда
//    3. SessionTimeFilter     — торговая сессия
//    4. MoneyRiskCalculator   — сырой лот от % риска
//    5. BrokerLotRounder      — нормализация лота
//    6. MarginCallChecker     — свободная маржа и MarginLevel
//    7. TP расчёт (2:1 если сигнал не задал)
//
//  Возвращает core::ValidatedOrderRequest с заполненными полями.
//  При отклонении: is_approved=false + reject_reason + reject_note.
// =============================================================================
#pragma once
#include "core/Types.h"
#include "core/Constants.h"
#include "patterns/PatternAggregator.h"
#include "validation/SpreadFilter.h"
#include "validation/CounterTrendPUChecker.h"
#include "validation/MoneyRiskCalculator.h"
#include "validation/BrokerLotRounder.h"
#include "validation/MarginCallChecker.h"
#include "validation/SessionTimeFilter.h"
#include <string>
namespace spartak::validation {
// -----------------------------------------------------------------------------
// Состояние счёта для валидации.
// -----------------------------------------------------------------------------
struct AccountSnapshot {
    double  balance              = 10'000.0;
    double  equity               = 10'000.0;
    double  free_margin          = 10'000.0;
    double  margin_used          = 0.0;
    double  leverage             = 500.0;
    double  commission_per_lot   = 5.0;
    double  min_margin_level_pct = 5'000.0;
};
// -----------------------------------------------------------------------------
// Результат валидации: approved-запрос + диагностика отказа.
// -----------------------------------------------------------------------------
struct ValidationResult {
    bool                         is_approved = false;
    core::RejectReason           reason      = core::RejectReason::None;
    std::string                  note;
    core::ValidatedOrderRequest  order;   // заполнено только при is_approved
    double                       rr_ratio    = 0.0;
    double                       risk_amount = 0.0;
};
// -----------------------------------------------------------------------------
// Полный конфиг валидатора.
// -----------------------------------------------------------------------------
struct SignalValidatorConfig {
    double          point                = 0.00001;
    double          contract_size        = 100'000.0;
    double          risk_percent         = 1.0;
    double          tp_risk_ratio        = 2.0;
    bool            use_signal_tp        = true;   // если сигнал дал TP — оставить
    SpreadFilterConfig        spread;
    CounterTrendConfig        counter_trend;
    MoneyRiskConfig           money;
    LotRounderConfig          lot_rounder;
    MarginConfig              margin;
    SessionConfig             session;
};
// -----------------------------------------------------------------------------
// SignalValidator — stateful (держит фильтры).
// -----------------------------------------------------------------------------
class SignalValidator {
public:
    explicit SignalValidator(SignalValidatorConfig cfg = {});
    // Главный вход.
    //   sig — сигнал из PatternAggregator
    //   ctx — рыночный контекст
    //   bar — текущий бар (для спреда, времени)
    //   acc — состояние счёта
    [[nodiscard]] ValidationResult validate(
        const core::PatternSignal&  sig,
        const core::MarketContext&  ctx,
        const core::Bar&            bar,
        const AccountSnapshot&      acc) const;
    const SignalValidatorConfig& config() const noexcept { return cfg_; }
private:
    SignalValidatorConfig       cfg_;
    SpreadFilter                spread_;
    CounterTrendPUChecker       counter_;
    MoneyRiskCalculator         money_;
    BrokerLotRounder            lot_rounder_;
    MarginCallChecker           margin_;
    SessionTimeFilter           session_;
};
} // namespace spartak::validation