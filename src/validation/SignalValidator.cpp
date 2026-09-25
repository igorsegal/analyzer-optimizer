#include "validation/SignalValidator.h"
#include <cmath>
#include <stdexcept>
namespace spartak::validation {
// -----------------------------------------------------------------------------
// Конструктор: собираем все фильтры из конфига.
// -----------------------------------------------------------------------------
SignalValidator::SignalValidator(SignalValidatorConfig cfg)
    : cfg_(cfg),
      spread_(cfg.spread),
      counter_(cfg.counter_trend),
      money_(cfg.money),
      lot_rounder_(cfg.lot_rounder),
      margin_(cfg.margin),
      session_(cfg.session)
{
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("SignalValidatorConfig::point must be > 0");
    if (cfg_.contract_size <= 0.0)
        throw std::invalid_argument("SignalValidatorConfig::contract_size must be > 0");
    if (cfg_.risk_percent <= 0.0 || cfg_.risk_percent > 100.0)
        throw std::invalid_argument("SignalValidatorConfig::risk_percent must be in (0,100]");
    if (cfg_.tp_risk_ratio <= 0.0)
        throw std::invalid_argument("SignalValidatorConfig::tp_risk_ratio must be > 0");
}
// -----------------------------------------------------------------------------
// Хелпер: создать rejected ValidationResult.
// -----------------------------------------------------------------------------
static ValidationResult reject(core::RejectReason r, std::string note) {
    ValidationResult v;
    v.is_approved = false;
    v.reason      = r;
    v.note        = std::move(note);
    return v;
}
// -----------------------------------------------------------------------------
// validate — основной пайплайн.
// -----------------------------------------------------------------------------
ValidationResult SignalValidator::validate(
        const core::PatternSignal&  sig,
        const core::MarketContext&  ctx,
        const core::Bar&            bar,
        const AccountSnapshot&      acc) const
{
    // 0) Есть ли вообще сигнал
    if (!sig.detected) {
        return reject(core::RejectReason::InvalidSignal, "no pattern signal");
    }
    // 1) Спред
    if (!spread_.pass(bar.spread)) {
        return reject(core::RejectReason::SpreadTooHigh,
                      "spread=" + std::to_string(bar.spread) +
                      " > max=" + std::to_string(cfg_.spread.max_points));
    }
    // 2) Сессия
    if (!session_.pass(bar.timestamp)) {
        return reject(core::RejectReason::SessionClosed, "outside trading session");
    }
    // 3) Тренд (Counter-trend PU check)
    const auto ct = counter_.check(sig.side, ctx.dominant_trend, sig.level, ctx);
    if (!ct.allowed) {
        return reject(core::RejectReason::TrendConflict,
                      ct.is_counter ? "counter-trend without PU" : "trend conflict");
    }
    // 4) Сырой лот (с учётом комиссии)
    const double raw_lot = money_.calcRawLotWithCommission(
        acc.balance, sig.trigger_price, sig.suggested_stop,
        acc.commission_per_lot);
    if (raw_lot <= 0.0) {
        return reject(core::RejectReason::BalanceTooLow, "raw_lot=0");
    }
    // 5) Нормализация лота
    const double lot = lot_rounder_.round_down(raw_lot);
    if (lot <= 0.0) {
        return reject(core::RejectReason::BelowMinLot,
                      "rounded lot < min_lot");
    }
    // 6) Маржа
    MarginAccountState marg_acc;
    marg_acc.equity              = acc.equity;
    marg_acc.free_margin         = acc.free_margin;
    marg_acc.margin_used         = acc.margin_used;
    marg_acc.leverage            = acc.leverage;
    marg_acc.min_margin_level_pct = acc.min_margin_level_pct;
    const auto marg_res = margin_.check(lot, marg_acc);
    switch (marg_res) {
        case MarginCheckResult::NotEnoughFreeMargin:
            return reject(core::RejectReason::MarginTooLow, "not enough free margin");
        case MarginCheckResult::MarginCall:
            return reject(core::RejectReason::MarginCall, "would trigger margin call");
        case MarginCheckResult::Ok:
            break;
    }
    // 7) Стоп с правильной стороны
    if (sig.side == core::OrderSide::Buy && sig.suggested_stop >= sig.trigger_price) {
        return reject(core::RejectReason::InvalidStop, "BUY stop >= entry");
    }
    if (sig.side == core::OrderSide::Sell && sig.suggested_stop <= sig.trigger_price) {
        return reject(core::RejectReason::InvalidStop, "SELL stop <= entry");
    }
    const double stop_dist = std::fabs(sig.trigger_price - sig.suggested_stop);
    if (stop_dist <= 0.0) {
        return reject(core::RejectReason::ZeroDistance, "entry == stop");
    }
    // 8) TP: сигнал мог задать свой (например, на следующем уровне).
    //    Если не задан -> рассчитываем 2:1 (tp_risk_ratio) от риска.
    //    В core::PatternSignal нет отдельного take_profit — есть level.
    //    В нашем текущем контракте используем rule: TP = entry ± (tp_risk_ratio * stop_dist).
    const double tp_ratio = cfg_.tp_risk_ratio;
    const double tp_dist  = stop_dist * tp_ratio;
    const double tp1_dist = stop_dist * 1.0;    // первый тейк на 1:1
    const double tp2_dist = tp_dist;            // второй — 2:1
    const double tp1 = (sig.side == core::OrderSide::Buy)
                     ? (sig.trigger_price + tp1_dist)
                     : (sig.trigger_price - tp1_dist);
    const double tp2 = (sig.side == core::OrderSide::Buy)
                     ? (sig.trigger_price + tp2_dist)
                     : (sig.trigger_price - tp2_dist);
    // 9) Сборка финального запроса
    ValidationResult v;
    v.is_approved = true;
    v.reason      = core::RejectReason::None;
    v.note        = "ok";
    v.order.is_approved    = true;
    v.order.side           = sig.side;
    v.order.volume         = lot;
    v.order.entry_price    = sig.trigger_price;
    v.order.stop_loss      = sig.suggested_stop;
    v.order.take_profit_1  = tp1;
    v.order.take_profit_2  = tp2;
    v.rr_ratio    = tp_ratio;
    v.risk_amount = stop_dist * lot * cfg_.contract_size;
    return v;
}
} // namespace spartak::validation