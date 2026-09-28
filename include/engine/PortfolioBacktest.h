// =============================================================================
//  SPARTAK :: engine/PortfolioBacktest.h
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

struct PortfolioConfig {
    double  initial_balance           = 10'000.0;
    double  risk_percent              = 0.5;
    double  min_margin_level_pct      = 2'000.0;
    double  min_rr                    = 1.34;
    std::size_t aggregate_bars        = 1;
    bool    verbose                   = true;
    bool    skip_false_breakout       = true;
    bool    skip_impulse_buy          = true;
    bool    skip_impulse_sell         = true;
    bool    skip_consolidation_buy    = true;
    bool    skip_consolidation_sell   = false;
    int64_t from_ms                   = 0;
    int64_t to_ms                     = 0;
    context::ContextAggregatorConfig   context;
    patterns::PatternAggregatorConfig  pattern;
    validation::SignalValidatorConfig  validation;
    position::PositionManagerConfig    position;
};

struct PortfolioReport {
    bool        ok = false;
    std::string error;
    std::size_t instruments_loaded    = 0;
    std::size_t instruments_failed    = 0;
    std::size_t bars_processed        = 0;
    std::size_t signals_total         = 0;
    std::size_t signals_skipped_type  = 0;
    std::size_t orders_approved       = 0;
    std::size_t orders_rejected       = 0;
    std::size_t orders_margin_blocked = 0;
    std::size_t reject_rr             = 0;
    std::size_t partial_closes        = 0;
    std::size_t full_closes           = 0;
    std::size_t wins                  = 0;
    std::size_t losses                = 0;
    double      total_win             = 0.0;
    double      total_loss            = 0.0;
    double      win_rate              = 0.0;
    double      profit_factor         = 0.0;
    double  initial_balance  = 0.0;
    double  final_balance    = 0.0;
    double  peak_equity      = 0.0;
    double  max_drawdown_pct = 0.0;
    double  return_pct       = 0.0;
    double  net_pnl          = 0.0;
    int64_t first_time_ms    = 0;
    int64_t last_time_ms     = 0;

    struct InstrumentResult {
        std::string symbol;
        std::size_t orders  = 0;
        double      net_pnl = 0.0;
    };
    std::vector<InstrumentResult> per_instrument;

    struct PatternResult {
        int         type      = 0;
        int         side      = 0;
        std::size_t orders    = 0;
        std::size_t wins      = 0;
        std::size_t losses    = 0;
        double      total_win = 0.0;
        double      total_loss = 0.0;
        double      net_pnl   = 0.0;
    };
    std::vector<PatternResult> per_pattern;

    struct YearResult {
        int         year      = 0;
        std::size_t orders    = 0;
        std::size_t wins      = 0;
        std::size_t losses    = 0;
        double      total_win = 0.0;
        double      total_loss = 0.0;
        double      net_pnl   = 0.0;
    };
    std::vector<YearResult> per_year;
};

class PortfolioBacktest {
public:
    explicit PortfolioBacktest(PortfolioConfig cfg = {});
    PortfolioReport run(const std::vector<std::string>& files);
    const PortfolioConfig& config() const noexcept { return cfg_; }
    void reset() { run_called_ = false; }
private:
    PortfolioConfig cfg_;
    bool run_called_ = false;
};

} // namespace spartak::engine