// =============================================================================
//  SPARTAK :: engine/PortfolioBacktest.h
//  Мульти-инструментальный бэктест с общим балансом.
//
//  Идея:
//    - N инструментов обрабатываются синхронно по timestamp.
//    - Каждый имеет свой ContextAnalyzer / PatternDetector / Validator / PM.
//    - Общий AccountState: все PnL стекаются в один баланс.
//    - Открытие новых ордеров блокируется при MarginLevel < min_margin_level_pct.
//
//  Запуск:  PortfolioBacktest::run(vector<path>, config) -> PortfolioReport
// =============================================================================
#pragma once
#include "core/Types.h"
#include "core/InstrumentSpec.h"
#include "core/Config.h"
#include "engine/BacktestPlayer.h"
#include "data/BarStream.h"
#include "context/ContextAggregator.h"
#include "patterns/PatternAggregator.h"
#include "validation/SignalValidator.h"
#include "position/PositionManager.h"
#include <cstdint>
#include <string>
#include <vector>
namespace spartak::engine {
// -----------------------------------------------------------------------------
// Конфиг портфеля.
// -----------------------------------------------------------------------------
struct PortfolioConfig {
    double  initial_balance           = 10'000.0;
    double  risk_percent              = 0.5;      // % риска на сделку
    double  min_margin_level_pct      = 2'000.0;  // блокируем ордера при MarginLevel < 2000%
    std::size_t aggregate_bars        = 1;        // 1 = H1 (файлы уже H1)
    bool    verbose                   = true;
    context::ContextAggregatorConfig   context;
    patterns::PatternAggregatorConfig  pattern;
    validation::SignalValidatorConfig  validation;
    position::PositionManagerConfig    position;

};
// -----------------------------------------------------------------------------
// Итоговый отчёт.
// -----------------------------------------------------------------------------
struct PortfolioReport {
    bool        ok = false;
    std::string error;
    std::size_t instruments_loaded = 0;
    std::size_t instruments_failed = 0;
    std::size_t bars_processed     = 0;
    std::size_t signals_total      = 0;
    std::size_t orders_approved    = 0;
    std::size_t orders_rejected    = 0;
    std::size_t orders_margin_blocked = 0;   // отклонено из-за MarginLevel
    double  initial_balance  = 0.0;
    double  final_balance    = 0.0;
    double  peak_equity      = 0.0;
    double  max_drawdown_pct = 0.0;
    double  return_pct       = 0.0;
    double  net_pnl          = 0.0;
    int64_t first_time_ms = 0;
    int64_t last_time_ms  = 0;
    struct InstrumentResult {
        std::string symbol;
        std::size_t orders  = 0;
        double      net_pnl = 0.0;
    };
    std::vector<InstrumentResult> per_instrument;
};
// -----------------------------------------------------------------------------
// PortfolioBacktest — stateful.
// -----------------------------------------------------------------------------
class PortfolioBacktest {
public:
    explicit PortfolioBacktest(PortfolioConfig cfg = {});
    // files — пути к H1 .bin файлам.
    PortfolioReport run(const std::vector<std::string>& files);
    const PortfolioConfig& config() const noexcept { return cfg_; }
private:
    PortfolioConfig cfg_;
};
} // namespace spartak::engine