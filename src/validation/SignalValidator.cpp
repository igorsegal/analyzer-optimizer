#include "validation/SignalValidator.h"
#include <cmath>
#include <algorithm>
#include <functional>
#include <vector>
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
    // 3b) ТВ-правило №4: стоп должен быть ЗА зоной (по ТЗ ч.6).
    // BUY: stop < zone_bottom. SELL: stop > zone_top.
    {
        const auto* match = static_cast<const core::PriceZone*>(nullptr);
        double best_d = 1e18;
        for (const auto& z : ctx.active_zones) {
            if (!z.is_active) continue;
            const double d = std::fabs(z.price_level - sig.level);
            if (d < best_d) { best_d = d; match = &z; }
        }
        if (match) {
            if (sig.side == core::OrderSide::Buy &&
                sig.suggested_stop >= match->zone_bottom) {
                return reject(core::RejectReason::InvalidStop,
                              "BUY stop not below zone_bottom");
            }
            if (sig.side == core::OrderSide::Sell &&
                sig.suggested_stop <= match->zone_top) {
                return reject(core::RejectReason::InvalidStop,
                              "SELL stop not above zone_top");
            }
        }
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
    // 8) TP из активных зон по направлению сделки.
    //    BUY:  3 ближайшие зоны выше entry, сортируем по возрастанию price_level
    //    SELL: 3 ближайшие зоны ниже entry, сортируем по убыванию price_level
    //    Если зон меньше 3 — fallback: entry ± stop_dist * {1, 2, 3}
    std::vector<double> tp_candidates;
    for (const auto& z : ctx.active_zones) {
        if (!z.is_active) continue;
        if (sig.side == core::OrderSide::Buy && z.price_level > sig.trigger_price)
            tp_candidates.push_back(z.price_level);
        if (sig.side == core::OrderSide::Sell && z.price_level < sig.trigger_price)
            tp_candidates.push_back(z.price_level);
    }
    if (sig.side == core::OrderSide::Buy)
        std::sort(tp_candidates.begin(), tp_candidates.end());
    else
        std::sort(tp_candidates.begin(), tp_candidates.end(), std::greater<double>());
    double tp1 = 0.0, tp2 = 0.0, tp3 = 0.0;
    if (tp_candidates.size() >= 3) {
        tp1 = tp_candidates[0];
        tp2 = tp_candidates[1];
        tp3 = tp_candidates[2];
    } else if (tp_candidates.size() == 2) {
        tp1 = tp_candidates[0];
        tp2 = tp_candidates[1];
        tp3 = (sig.side == core::OrderSide::Buy)
            ? (sig.trigger_price + stop_dist * 3.0)
            : (sig.trigger_price - stop_dist * 3.0);
    } else if (tp_candidates.size() == 1) {
        tp1 = tp_candidates[0];
        tp2 = (sig.side == core::OrderSide::Buy)
            ? (sig.trigger_price + stop_dist * 2.0)
            : (sig.trigger_price - stop_dist * 2.0);
        tp3 = (sig.side == core::OrderSide::Buy)
            ? (sig.trigger_price + stop_dist * 3.0)
            : (sig.trigger_price - stop_dist * 3.0);
    } else {
        tp1 = (sig.side == core::OrderSide::Buy)
            ? (sig.trigger_price + stop_dist * 1.0)
            : (sig.trigger_price - stop_dist * 1.0);
        tp2 = (sig.side == core::OrderSide::Buy)
            ? (sig.trigger_price + stop_dist * 2.0)
            : (sig.trigger_price - stop_dist * 2.0);
        tp3 = (sig.side == core::OrderSide::Buy)
            ? (sig.trigger_price + stop_dist * 3.0)
            : (sig.trigger_price - stop_dist * 3.0);
    }
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
    v.order.take_profit_3  = tp3;
    v.rr_ratio    = (stop_dist > 0.0) ? ((tp3 - sig.trigger_price) / stop_dist) : 0.0;
    v.risk_amount = stop_dist * lot * cfg_.contract_size;
    return v;
}
} // namespace spartak::validation