// =============================================================================
//  SPARTAK :: position/PositionStateSynchronizer.h
//  Синхронизация состояния позиции после операций.
//
//  Задача: единая точка правды — структура PositionState, которая
//  обновляется после каждого события (open / partial / BE / trail / close).
//
//  Все кластеры position/ читают и пишут ТОЛЬКО через этот класс,
//  чтобы избежать рассинхрона. Например, после partial close нужно:
//    - уменьшить remaining_volume;
//    - отметить tp1_hit = true;
//    - сохранить накопленный swap / commission.
//
//  PositionStateSynchronizer предоставляет атомарные операции
//  «открыть», «частично закрыть», «сдвинуть стоп», «полностью закрыть».
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstdint>
#include <string>
namespace spartak::position {
// -----------------------------------------------------------------------------
// Полное состояние позиции.
// -----------------------------------------------------------------------------
struct PositionState {
    uint64_t         id                = 0;
    core::OrderSide  side              = core::OrderSide::Buy;
    double           entry_price       = 0.0;
    double           initial_volume    = 0.0;
    double           remaining_volume  = 0.0;
    double           stop_loss         = 0.0;
    double           take_profit_1     = 0.0;
    double           take_profit_2     = 0.0;
    int64_t          open_time_ms      = 0;
    int64_t          close_time_ms     = 0;
    int32_t          entry_spread_pts  = 0;
    bool             tp1_hit           = false;
    bool             be_moved          = false;
    bool             closed            = false;
    double           realized_pnl      = 0.0;   // сумма по частичным
    double           total_commission  = 0.0;   // списанная комиссия (open + close)
    double           total_swap        = 0.0;   // начисленные свопы
    int64_t          last_swap_day     = -1;    // день последнего начисления
    std::string      close_reason;              // "tp1_partial", "tp2", "sl", "be", "manual"
    bool is_active() const noexcept {
        return !closed && remaining_volume > 1e-9;
    }
};
// -----------------------------------------------------------------------------
// PositionStateSynchronizer — stateless.
// -----------------------------------------------------------------------------
class PositionStateSynchronizer {
public:
    PositionStateSynchronizer() = default;
    // --- Операции над состоянием ---
    // Регистрация открытия.
    static void markOpened(PositionState& s,
                           double entry_price,
                           double volume,
                           double sl,
                           double tp1,
                           double tp2,
                           int32_t spread_pts,
                           int64_t open_time_ms) noexcept;
    // Частичное закрытие. tp1_done — отметить tp1_hit.
    static void markPartialClose(PositionState& s,
                                 double closed_volume,
                                 double close_price,
                                 double gross_pnl,
                                 double commission,
                                 double swap_part,
                                 bool   tp1_done) noexcept;
    // Сдвиг стопа (BE или trail). be_done — отметить be_moved.
    static void markStopMoved(PositionState& s,
                              double new_sl,
                              bool   be_done) noexcept;
    // Полное закрытие.
    static void markFullClose(PositionState& s,
                              double close_price,
                              double gross_pnl,
                              double commission,
                              double swap_part,
                              int64_t close_time_ms,
                              std::string reason) noexcept;
    // Обновление swap-аккумулятора.
    static void accrueSwap(PositionState& s,
                           double money,
                           int64_t new_day) noexcept;
    // Утилита: реальная PnL (gross - commission - swap).
    static double netPnl(const PositionState& s) noexcept;
};
} // namespace spartak::position