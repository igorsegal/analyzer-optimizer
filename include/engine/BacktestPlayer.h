// =============================================================================
//  SPARTAK :: engine/BacktestPlayer.h
//  Главный цикл бэктеста.
//
//  Связывает все 4 слоя в один поток:
//    Bar -> ContextAggregator -> PatternAggregator
//             -> SignalValidator -> PositionManager
//
//  Ведёт:
//    - balance (изменяется по net PnL каждой сделки)
//    - equity / free_margin (пересчитывается на каждом баре)
//    - статистику по сделкам
//
//  Вход: путь к XFBAR-файлу + конфиги всех слоёв.
//  Выход: BacktestReport с полной сводкой.
// =============================================================================
#pragma once
#include "core/Types.h"
#include "data/BarStream.h"
#include "data/DataSanitizer.h"
#include "context/ContextAggregator.h"
#include "patterns/PatternAggregator.h"
#include "validation/SignalValidator.h"
#include "position/PositionManager.h"
#include <cstdint>
#include <string>
#include <vector>
namespace spartak::engine {
// -----------------------------------------------------------------------------
// Полная конфигурация бэктеста.
// -----------------------------------------------------------------------------
struct BacktestConfig {
    // Общие
    double  initial_balance = 10'000.0;
    double  point           = 0.00001;
    double  contract_size   = 100'000.0;
    double  leverage        = 500.0;
    double  commission_per_lot = 5.0;
    double  min_margin_level_pct = 5'000.0;
    // Окно анализа
    std::size_t rolling_window = 100;    // сколько баров подавать в ContextAggregator
    // Пропускать ли нерегулярный префикс данных
    bool skip_irregular_prefix = true;
    int64_t max_gap_ms         = 14'400'000LL;   // 4 часа
    // Ограничения
    std::size_t max_bars       = 0;      // 0 = читать все бары
    // Вложенные конфиги слоёв
    context::ContextAggregatorConfig   context;
    patterns::PatternAggregatorConfig  pattern;
    validation::SignalValidatorConfig  validation;
    position::PositionManagerConfig    position;
    // Логирование
    bool   verbose           = true;
    std::size_t log_every_n  = 0;        // 0 = не логировать прогресс
};
// -----------------------------------------------------------------------------
// Отчёт о прогоне.
// -----------------------------------------------------------------------------
struct BacktestReport {
    bool    ok                     = false;
    std::string error;
    std::string symbol;
    int64_t  period_seconds        = 0;
    int64_t  bars_total            = 0;
    int64_t  bars_skipped_prefix   = 0;
    int64_t  bars_processed        = 0;
    // Сигналы и ордера
    std::size_t signals_detected   = 0;
    std::size_t orders_approved    = 0;
    std::size_t orders_rejected    = 0;
    // Сделки
    std::size_t partial_closes     = 0;
    std::size_t full_closes        = 0;
    std::size_t be_moves           = 0;
    std::size_t trailing_moves     = 0;
    std::size_t swaps_accrued      = 0;
    // Финансы
    double  initial_balance        = 0.0;
    double  final_balance          = 0.0;
    double  net_pnl                = 0.0;
    double  return_pct             = 0.0;
    double  max_drawdown_pct       = 0.0;
    double  peak_balance           = 0.0;
    double  total_commission       = 0.0;
    double  total_swap             = 0.0;
    // Первые N сделок
    std::vector<position::PositionEvent> sample_events;
};
// -----------------------------------------------------------------------------
// BacktestPlayer — stateful.
// -----------------------------------------------------------------------------
class BacktestPlayer {
public:
    explicit BacktestPlayer(BacktestConfig cfg = {});
    // Главный вход: прогон по файлу.
    BacktestReport run(const std::string& xfbar_path);
    const BacktestConfig& config() const noexcept { return cfg_; }
private:
    BacktestConfig cfg_;
};
} // namespace spartak::engine