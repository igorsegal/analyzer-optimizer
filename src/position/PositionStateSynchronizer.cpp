#include "position/PositionStateSynchronizer.h"
#include <algorithm>
#include <utility>
namespace spartak::position {
// -----------------------------------------------------------------------------
// markOpened — фиксируем открытие.
// -----------------------------------------------------------------------------
void PositionStateSynchronizer::markOpened(PositionState& s,
                                           double entry_price,
                                           double volume,
                                           double sl,
                                           double tp1,
                                           double tp2,
                                           int32_t spread_pts,
                                           int64_t open_time_ms) noexcept
{
    s.entry_price       = entry_price;
    s.initial_volume    = volume;
    s.remaining_volume  = volume;
    s.stop_loss         = sl;
    s.take_profit_1     = tp1;
    s.take_profit_2     = tp2;
    s.entry_spread_pts  = spread_pts;
    s.open_time_ms      = open_time_ms;
    s.tp1_hit           = false;
    s.be_moved          = false;
    s.closed            = false;
}
// -----------------------------------------------------------------------------
// markPartialClose — уменьшаем объём, копим PnL / комиссию / swap.
// -----------------------------------------------------------------------------
void PositionStateSynchronizer::markPartialClose(PositionState& s,
                                                 double closed_volume,
                                                 double /*close_price*/,
                                                 double gross_pnl,
                                                 double commission,
                                                 double swap_part,
                                                 bool   tp1_done) noexcept
{
    s.remaining_volume = std::max(0.0, s.remaining_volume - closed_volume);
    s.realized_pnl    += gross_pnl;
    s.total_commission += commission;
    s.total_swap      -= swap_part;   // swap_part «списывается» из накопленного
    if (tp1_done) s.tp1_hit = true;
    if (s.remaining_volume < 1e-9) {
        s.remaining_volume = 0.0;
        s.closed = true;
    }
}
// -----------------------------------------------------------------------------
// markStopMoved — сдвигаем стоп, отмечаем BE.
// -----------------------------------------------------------------------------
void PositionStateSynchronizer::markStopMoved(PositionState& s,
                                              double new_sl,
                                              bool   be_done) noexcept
{
    s.stop_loss = new_sl;
    if (be_done) s.be_moved = true;
}
// -----------------------------------------------------------------------------
// markFullClose — закрываем всё, фиксируем причину.
// -----------------------------------------------------------------------------
void PositionStateSynchronizer::markFullClose(PositionState& s,
                                              double /*close_price*/,
                                              double gross_pnl,
                                              double commission,
                                              double swap_part,
                                              int64_t close_time_ms,
                                              std::string reason) noexcept
{
    s.realized_pnl    += gross_pnl;
    s.total_commission += commission;
    s.total_swap      -= swap_part;
    s.remaining_volume = 0.0;
    s.closed           = true;
    s.close_time_ms    = close_time_ms;
    s.close_reason     = std::move(reason);
}
// -----------------------------------------------------------------------------
// accrueSwap — добавить начисленный своп.
// -----------------------------------------------------------------------------
void PositionStateSynchronizer::accrueSwap(PositionState& s,
                                           double money,
                                           int64_t new_day) noexcept
{
    s.total_swap += money;
    s.last_swap_day = new_day;
}
// -----------------------------------------------------------------------------
// netPnl — итоговый PnL с вычетом издержек.
// -----------------------------------------------------------------------------
double PositionStateSynchronizer::netPnl(const PositionState& s) noexcept {
    return s.realized_pnl - s.total_commission + s.total_swap;
}
} // namespace spartak::position