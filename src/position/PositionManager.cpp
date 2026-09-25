#include "position/PositionManager.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace spartak::position {
// -----------------------------------------------------------------------------
// to_string(PositionEventType)
// -----------------------------------------------------------------------------
const char* to_string(PositionEventType t) noexcept {
    switch (t) {
        case PositionEventType::Opened:        return "Opened";
        case PositionEventType::PartialClose:  return "PartialClose";
        case PositionEventType::MoveBreakEven: return "MoveBreakEven";
        case PositionEventType::TrailingMove:  return "TrailingMove";
        case PositionEventType::FullClose:     return "FullClose";
        case PositionEventType::SwapAccrued:   return "SwapAccrued";
    }
    return "Unknown";
}
// -----------------------------------------------------------------------------
// Конструктор.
// -----------------------------------------------------------------------------
PositionManager::PositionManager(PositionManagerConfig cfg)
    : cfg_(cfg),
      uo1_(),
      splitter_(cfg.splitter),
      breakeven_(cfg.breakeven),
      swap_tracker_(cfg.swap),
      commission_(cfg.commission),
      trailing_(cfg.trailing),
      emergency_(cfg.emergency)
{
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("PositionManagerConfig::point must be > 0");
    if (cfg_.contract_size <= 0.0)
        throw std::invalid_argument("PositionManagerConfig::contract_size must be > 0");
}
// -----------------------------------------------------------------------------
// openPosition
// -----------------------------------------------------------------------------
uint64_t PositionManager::openPosition(const core::ValidatedOrderRequest& order,
                                       int32_t spread_pts,
                                       int64_t open_time_ms)
{
    PositionState s;
    s.id              = next_id_++;
    s.side            = order.side;
    PositionStateSynchronizer::markOpened(
        s,
        order.entry_price,
        order.volume,
        order.stop_loss,
        order.take_profit_1,
        order.take_profit_2,
        spread_pts,
        open_time_ms);
    // Фиксируем день открытия для свопов
    s.last_swap_day = SwapAccrualTracker::day_of(open_time_ms);
    positions_.push_back(s);
    return s.id;
}
// -----------------------------------------------------------------------------
// closePartial — частичное закрытие.
// -----------------------------------------------------------------------------
PositionEvent PositionManager::closePartial(PositionState& s,
                                            double close_volume,
                                            double close_price,
                                            int64_t now_ms,
                                            std::string reason)
{
    PositionEvent e;
    e.type        = PositionEventType::PartialClose;
    e.position_id = s.id;
    e.side        = s.side;
    e.price       = close_price;
    e.volume      = close_volume;
    e.time_ms     = now_ms;
    e.reason      = std::move(reason);
    // gross pnl
    const double dir = (s.side == core::OrderSide::Buy) ? 1.0 : -1.0;
    const double gross = (close_price - s.entry_price) * dir
                       * close_volume * cfg_.contract_size;
    // комиссия пропорциональна закрываемому объёму
    const double comm = commission_.closeCommission(close_volume);
    // своп пропорционально закрываемой доле от initial
    const double portion = (s.initial_volume > 0.0)
                         ? (close_volume / s.initial_volume)
                         : 0.0;
    const double swap_part = s.total_swap * portion;
    e.gross_pnl  = gross;
    e.commission = comm;
    e.swap       = swap_part;
    e.net_pnl    = gross - comm - swap_part;
    const bool tp1_done = (e.reason == "tp1");
    PositionStateSynchronizer::markPartialClose(
        s, close_volume, close_price, gross, comm, swap_part, tp1_done);
    total_realized_pnl_ += e.net_pnl;
    return e;
}
// -----------------------------------------------------------------------------
// closeFull — полное закрытие.
// -----------------------------------------------------------------------------
PositionEvent PositionManager::closeFull(PositionState& s,
                                         double close_price,
                                         int64_t now_ms,
                                         std::string reason)
{
    PositionEvent e;
    e.type        = PositionEventType::FullClose;
    e.position_id = s.id;
    e.side        = s.side;
    e.price       = close_price;
    e.volume      = s.remaining_volume;
    e.time_ms     = now_ms;
    e.reason      = std::move(reason);
    const double dir = (s.side == core::OrderSide::Buy) ? 1.0 : -1.0;
    const double gross = (close_price - s.entry_price) * dir
                       * s.remaining_volume * cfg_.contract_size;
    const double comm = commission_.closeCommission(s.remaining_volume);
    const double swap_part = s.total_swap;
    e.gross_pnl  = gross;
    e.commission = comm;
    e.swap       = swap_part;
    e.net_pnl    = gross - comm - swap_part;
    PositionStateSynchronizer::markFullClose(
        s, close_price, gross, comm, swap_part, now_ms, e.reason);
    total_realized_pnl_ += e.net_pnl;
    return e;
}
// -----------------------------------------------------------------------------
// moveStop — BE или trail.
// -----------------------------------------------------------------------------
PositionEvent PositionManager::moveStop(PositionState& s,
                                        double new_sl,
                                        bool is_be,
                                        int64_t now_ms,
                                        std::string reason)
{
    PositionEvent e;
    e.type        = is_be ? PositionEventType::MoveBreakEven
                          : PositionEventType::TrailingMove;
    e.position_id = s.id;
    e.side        = s.side;
    e.price       = s.entry_price;   // не используется, но информативно
    e.new_sl      = new_sl;
    e.time_ms     = now_ms;
    e.reason      = std::move(reason);
    PositionStateSynchronizer::markStopMoved(s, new_sl, is_be);
    return e;
}
// -----------------------------------------------------------------------------
// onBar — главный цикл.
//
// Для каждой активной позиции:
//   1. Emergency check (margin / drawdown / max holding)
//   2. Swap accrual (полночь)
//   3. UO1Trigger — что произошло на баре?
//   4. Обработка событий: SL -> FullClose, TP2 -> FullClose, TP1 -> partial + BE
//   5. Trailing (если включён и TP1 не сработал — трейлинг ДО TP1 = защита)
// -----------------------------------------------------------------------------
std::vector<PositionEvent> PositionManager::onBar(const core::Bar& bar,
                                                  const PositionAccountContext& acc)
{
    std::vector<PositionEvent> events;
    for (auto& s : positions_) {
        if (!s.is_active()) continue;
        // --- 0. Emergency check ---
        if (cfg_.emergency_enabled) {
            EmergencyContext ectx;
            ectx.margin_level_pct = acc.margin_level_pct;
            ectx.risk_money       = std::fabs(s.entry_price - s.stop_loss)
                                  * s.initial_volume * cfg_.contract_size;
            // current_pnl: считаем приблизительно по mid-цене close
            const double dir = (s.side == core::OrderSide::Buy) ? 1.0 : -1.0;
            ectx.current_pnl = (bar.close - s.entry_price) * dir
                             * s.remaining_volume * cfg_.contract_size
                             + s.total_swap;
            ectx.open_time_ms = s.open_time_ms;
            ectx.now_ms       = bar.timestamp;
            auto d = emergency_.check(ectx);
            if (d.should_close) {
                events.push_back(closeFull(s, bar.close, bar.timestamp,
                                           std::string("emergency:") +
                                           to_string(d.reason)));
                continue;
            }
        }
        // --- 1. Swap accrual ---
        {
            auto acc_res = swap_tracker_.compute(
                s.side, s.remaining_volume, s.last_swap_day, bar.timestamp);
            if (acc_res.accrued) {
                PositionStateSynchronizer::accrueSwap(
                    s, acc_res.money, acc_res.new_day);
                PositionEvent e;
                e.type        = PositionEventType::SwapAccrued;
                e.position_id = s.id;
                e.side        = s.side;
                e.swap        = acc_res.money;
                e.time_ms     = bar.timestamp;
                e.reason      = "swap_" + std::to_string(acc_res.nights) + "n";
                events.push_back(e);
            }
        }
        // --- 2. UO1 trigger ---
        TriggerContext tctx;
        tctx.side    = s.side;
        tctx.sl      = s.stop_loss;
        tctx.tp1     = s.take_profit_1;
        tctx.tp2     = s.take_profit_2;
        tctx.tp1_hit = s.tp1_hit;
        tctx.tp2_hit = false;   // у нас только один tp2
        auto ev = uo1_.check(bar, tctx);
        if (ev == TriggerEvent::SL_Hit) {
            events.push_back(closeFull(s, s.stop_loss, bar.timestamp, "sl"));
            continue;
        }
        if (ev == TriggerEvent::TP2_Hit) {
            events.push_back(closeFull(s, s.take_profit_2, bar.timestamp, "tp2"));
            continue;
        }
        if (ev == TriggerEvent::TP1_Hit) {
            // Part 1: частичное закрытие
            auto sr = splitter_.split(s.initial_volume, s.remaining_volume);
            if (!sr.ok) continue;   // ничего не делаем
            if (sr.full_close) {
                events.push_back(closeFull(s, s.take_profit_1, bar.timestamp,
                                           "tp1_full"));
                continue;
            }
            events.push_back(closePartial(s, sr.close_lot, s.take_profit_1,
                                          bar.timestamp, "tp1"));
            // Part 2: BE
            auto be_res = breakeven_.compute(
                s.side, s.entry_price, s.take_profit_1,
                s.entry_spread_pts, s.stop_loss);
            if (be_res.ok) {
                events.push_back(moveStop(s, be_res.new_sl, true, bar.timestamp, "be"));
            }
            continue;
        }
        // --- 3. Trailing (если разрешён и не BE) ---
        if (cfg_.use_trailing) {
            auto tr = trailing_.compute(s.side, s.stop_loss,
                                        s.entry_price, bar);
            if (tr.moved) {
                events.push_back(moveStop(s, tr.new_sl, false,
                                          bar.timestamp, "trail"));
            }
        }
    }
    // Удаляем закрытые
    positions_.erase(
        std::remove_if(positions_.begin(), positions_.end(),
                       [](const PositionState& s) { return s.closed; }),
        positions_.end());
    return events;
}
// -----------------------------------------------------------------------------
// forceClose — ручное/принудительное закрытие.
// -----------------------------------------------------------------------------
PositionEvent PositionManager::forceClose(uint64_t id,
                                          double price,
                                          int64_t now_ms,
                                          std::string reason)
{
    auto it = std::find_if(positions_.begin(), positions_.end(),
                           [id](const PositionState& s) { return s.id == id; });
    if (it == positions_.end()) return {};
    auto e = closeFull(*it, price, now_ms, std::move(reason));
    positions_.erase(
        std::remove_if(positions_.begin(), positions_.end(),
                       [](const PositionState& s) { return s.closed; }),
        positions_.end());
    return e;
}
// -----------------------------------------------------------------------------
// Доступ.
// -----------------------------------------------------------------------------
std::size_t PositionManager::activeCount() const noexcept {
    std::size_t n = 0;
    for (const auto& s : positions_) if (s.is_active()) ++n;
    return n;
}
const PositionState* PositionManager::find(uint64_t id) const noexcept {
    auto it = std::find_if(positions_.begin(), positions_.end(),
                           [id](const PositionState& s) { return s.id == id; });
    return (it == positions_.end()) ? nullptr : &(*it);
}
double PositionManager::realizedTotal() const noexcept {
    // Возвращаем глобальный аккумулятор (закрытые сделки уже удалены).
    // Дополнительно прибавляем PnL по ещё открытым позициям.
    double live = 0.0;
    for (const auto& s : positions_) {
        live += PositionStateSynchronizer::netPnl(s);
    }
    return total_realized_pnl_ + live;
}
} // namespace spartak::position