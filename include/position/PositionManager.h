// =============================================================================
//  SPARTAK :: position/PositionManager.h
//  Финальный агрегатор слоя position/.
//
//  Хранит список открытых позиций и на каждом баре прогоняет 8 кластеров:
//    1. UO1Trigger              — детект SL/TP1/TP2
//    2. VolumeSplitter50        — split на partial close
//    3. BreakEvenTransfer       — новый SL после TP1
//    4. TrailingStopManager     — трейлинг
//    5. SwapAccrualTracker      — свопы через полночь
//    6. CloseCommissionCalculator — комиссия за закрытие
//    7. PositionStateSynchronizer — обновление состояния
//    8. EmergencyCloseHandler   — аварийное закрытие
//
//  На выходе каждого бара — список PositionEvent, описывающих что произошло.
//  BacktestPlayer применяет события к балансу (через ExecutionEngine / cash-flow).
// =============================================================================
#pragma once
#include "core/Types.h"
#include "position/UO1Trigger.h"
#include "position/VolumeSplitter50.h"
#include "position/BreakEvenTransfer.h"
#include "position/SwapAccrualTracker.h"
#include "position/CloseCommissionCalculator.h"
#include "position/TrailingStopManager.h"
#include "position/PositionStateSynchronizer.h"
#include "position/EmergencyCloseHandler.h"
#include <cstdint>
#include <string>
#include <vector>
namespace spartak::position {
// -----------------------------------------------------------------------------
// Тип события по позиции.
// -----------------------------------------------------------------------------
enum class PositionEventType {
    Opened,
    PartialClose,   // закрыли часть по TP1
    MoveBreakEven,  // перенесли SL в БУ
    TrailingMove,   // трейлинг сдвинул SL
    FullClose,      // закрыли всё
    SwapAccrued     // только начислили своп (без изменения позиции)
};
[[nodiscard]] const char* to_string(PositionEventType t) noexcept;
// -----------------------------------------------------------------------------
// Событие по позиции.
// -----------------------------------------------------------------------------
struct PositionEvent {
    PositionEventType type        = PositionEventType::Opened;
    uint64_t          position_id = 0;
    core::OrderSide   side        = core::OrderSide::Buy;
    double            price       = 0.0;   // цена события
    double            volume      = 0.0;   // объём (для close)
    double            new_sl      = 0.0;   // для BE/trail
    double            gross_pnl   = 0.0;   // без издержек
    double            commission  = 0.0;   // списано
    double            swap        = 0.0;   // своп за эту операцию
    double            net_pnl     = 0.0;   // итог
    std::string       reason;              // "tp1", "tp2", "sl", "be", "trail", "margin_call", ...
    int64_t           time_ms     = 0;
};
// -----------------------------------------------------------------------------
// Конфиг агрегатора.
// -----------------------------------------------------------------------------
struct PositionManagerConfig {
    double  point              = 0.00001;
    double  contract_size      = 100'000.0;
    VolumeSplitterConfig      splitter;
    BreakEvenConfig           breakeven;
    SwapConfig                swap;
    CommissionConfig          commission;
    TrailingConfig            trailing;
    EmergencyConfig           emergency;
    bool    use_trailing        = true;    // включать/выключать трейлинг
    bool    emergency_enabled   = true;    // включать/выключать emergency
};
// -----------------------------------------------------------------------------
// Счёт (для emergency).
// -----------------------------------------------------------------------------
struct PositionAccountContext {
    double  margin_level_pct = 0.0;   // для emergency margin call
    int64_t now_ms           = 0;
};
// -----------------------------------------------------------------------------
// PositionManager — stateful.
// -----------------------------------------------------------------------------
class PositionManager {
public:
    explicit PositionManager(PositionManagerConfig cfg = {});
    // Открыть позицию из ValidatedOrderRequest.
    uint64_t openPosition(const core::ValidatedOrderRequest& order,
                          int32_t spread_pts,
                          int64_t open_time_ms);
    // Главный вход: обработка бара.
    std::vector<PositionEvent> onBar(const core::Bar& bar,
                                     const PositionAccountContext& acc);
    // Принудительное закрытие (manual).
    PositionEvent forceClose(uint64_t id,
                             double price,
                             int64_t now_ms,
                             std::string reason);
    // Доступ.
    std::size_t activeCount() const noexcept;
    const PositionState* find(uint64_t id) const noexcept;
    const std::vector<PositionState>& positions() const noexcept { return positions_; }
    // Итог.
    double realizedTotal() const noexcept;
    const PositionManagerConfig& config() const noexcept { return cfg_; }
private:
    PositionManagerConfig          cfg_;
    std::vector<PositionState>     positions_;
    uint64_t                       next_id_ = 1;
    double                         total_realized_pnl_ = 0.0;   // аккумулированный PnL
    UO1Trigger                     uo1_;
    VolumeSplitter50               splitter_;
    BreakEvenTransfer              breakeven_;
    SwapAccrualTracker             swap_tracker_;
    CloseCommissionCalculator      commission_;
    TrailingStopManager            trailing_;
    EmergencyCloseHandler          emergency_;
    // --- Внутренние операции ---
    PositionEvent closePartial(PositionState& s,
                               double close_volume,
                               double close_price,
                               int64_t now_ms,
                               std::string reason);
    PositionEvent closeFull(PositionState& s,
                            double close_price,
                            int64_t now_ms,
                            std::string reason);
    PositionEvent moveStop(PositionState& s,
                           double new_sl,
                           bool is_be,
                           int64_t now_ms,
                           std::string reason);
};
} // namespace spartak::position