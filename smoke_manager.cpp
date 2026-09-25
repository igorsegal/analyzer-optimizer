// Smoke: PositionManager — ПОЛНЫЙ end-to-end pipeline.
// Bar -> context -> patterns -> validation -> position.
#include "data/BarStream.h"
#include "data/DataSanitizer.h"
#include "context/ContextAggregator.h"
#include "patterns/PatternAggregator.h"
#include "validation/SignalValidator.h"
#include "position/PositionManager.h"
#include <iostream>
#include <iomanip>
#include <vector>
using namespace spartak;
static const char* event_name(position::PositionEventType t) {
    return position::to_string(t);
}
int main(int argc, char** argv) {
    const std::string path = (argc > 1)
        ? argv[1]
        : "D:/AHexaTrader/1DataFiles/raw/EURUSD/EURUSD_M5_real.bin";
    std::cout << "=== PositionManager END-TO-END smoke test ===\n\n";
    data::BarStream stream(path);
    if (!stream.is_ok()) { std::cout << "Stream open FAILED\n"; return 1; }
    data::DataSanitizer sanitizer;
    (void)sanitizer.run(stream);
    constexpr int N = 2000;
    std::vector<core::Bar> bars;
    bars.reserve(N);
    core::Bar b;
    while (bars.size() < N && stream.next(b)) bars.push_back(b);
    std::cout << "Bars loaded : " << bars.size() << "\n\n";
    // --- Config ---
    context::ContextAggregatorConfig ctx_cfg;
    ctx_cfg.point              = 0.00001;
    ctx_cfg.fractal_radius     = 2;
    ctx_cfg.max_zones_per_tf   = 5;
    ctx_cfg.use_same_tf_for_both = true;
    patterns::PatternAggregatorConfig pat_cfg;
    pat_cfg.point = 0.00001;
    pat_cfg.volume.min_volume = 0;
    pat_cfg.volume.avg_ratio  = 0.0;
    pat_cfg.require_trend_for_consolidation = false;
    validation::SignalValidatorConfig val_cfg;
    val_cfg.point          = 0.00001;
    val_cfg.contract_size  = 100'000.0;
    val_cfg.risk_percent   = 1.0;
    val_cfg.tp_risk_ratio  = 2.0;
    val_cfg.spread.max_points = 30;
    val_cfg.counter_trend.allow_counter_trend_on_pu = true;
    val_cfg.counter_trend.point = 0.00001;
    val_cfg.money.risk_percent  = 1.0;
    val_cfg.money.contract_size = 100'000.0;
    val_cfg.money.point         = 0.00001;
    val_cfg.lot_rounder.min_lot  = 0.01;
    val_cfg.lot_rounder.max_lot  = 1.0;
    val_cfg.lot_rounder.lot_step = 0.01;
    val_cfg.margin.contract_size = 100'000.0;
    val_cfg.session.enabled = false;
    position::PositionManagerConfig pm_cfg;
    pm_cfg.point            = 0.00001;
    pm_cfg.contract_size    = 100'000.0;
    pm_cfg.splitter.partial_pct = 0.50;
    pm_cfg.splitter.min_lot  = 0.01;
    pm_cfg.splitter.lot_step = 0.01;
    pm_cfg.breakeven.point = 0.00001;
    pm_cfg.breakeven.use_spread_in_be = true;
    pm_cfg.swap.point = 0.00001;
    pm_cfg.swap.contract_size = 100'000.0;
    pm_cfg.swap.swap_long_points  = -7.0;
    pm_cfg.swap.swap_short_points = -2.0;
    pm_cfg.commission.commission_per_lot = 5.0;
    pm_cfg.trailing.point = 0.00001;
    pm_cfg.trailing.trailing_distance_points = 30;
    pm_cfg.trailing.activation_points = 50;
    pm_cfg.use_trailing = true;
    pm_cfg.emergency_enabled = false;   // выключено для чистого теста
    context::ContextAggregator    ctx_agg(ctx_cfg);
    patterns::PatternAggregator   pat_agg(pat_cfg);
    validation::SignalValidator   val(val_cfg);
    position::PositionManager     pm(pm_cfg);
    validation::AccountSnapshot   acc;
    acc.balance              = 10'000.0;
    acc.equity               = 10'000.0;
    acc.free_margin          = 10'000.0;
    acc.margin_used          = 0.0;
    acc.leverage             = 500.0;
    acc.commission_per_lot   = 5.0;
    acc.min_margin_level_pct = 5'000.0;
    constexpr std::size_t WINDOW = 100;
    // Статистика
    std::size_t signals_total = 0;
    std::size_t orders_sent   = 0;
    std::size_t events_count  = 0;
    std::size_t partial = 0, full = 0, be = 0, trail = 0, swap_ev = 0;
    for (std::size_t i = WINDOW; i < bars.size(); ++i) {
        std::vector<core::Bar> slice(bars.begin() + (i - WINDOW), bars.begin() + i + 1);
        auto mctx = ctx_agg.analyze({}, slice);
        auto sig  = pat_agg.analyze(slice, mctx);
        if (!sig.detected) {
            // обработаем бар для существующих позиций
            auto evs = pm.onBar(slice.back(), {5000.0, slice.back().timestamp});
            events_count += evs.size();
            continue;
        }
        ++signals_total;
        auto r = val.validate(sig, mctx, slice.back(), acc);
        if (r.is_approved) {
            pm.openPosition(r.order, slice.back().spread, slice.back().timestamp);
            ++orders_sent;
        }
        // Прогоняем бар через PositionManager
        position::PositionAccountContext pac;
        pac.margin_level_pct = 5000.0;
        pac.now_ms           = slice.back().timestamp;
        auto evs = pm.onBar(slice.back(), pac);
        events_count += evs.size();
        for (const auto& e : evs) {
            switch (e.type) {
                case position::PositionEventType::PartialClose:  ++partial;  break;
                case position::PositionEventType::FullClose:     ++full;     break;
                case position::PositionEventType::MoveBreakEven: ++be;       break;
                case position::PositionEventType::TrailingMove:  ++trail;    break;
                case position::PositionEventType::SwapAccrued:   ++swap_ev;  break;
                default: break;
            }
        }
    }
    std::cout << "--- Summary ---\n";
    std::cout << "  Bars processed     : " << (bars.size() - WINDOW) << "\n";
    std::cout << "  Signals detected   : " << signals_total << "\n";
    std::cout << "  Orders sent to PM  : " << orders_sent << "\n";
    std::cout << "  Position events    : " << events_count << "\n";
    std::cout << "    PartialClose     : " << partial << "\n";
    std::cout << "    MoveBreakEven    : " << be << "\n";
    std::cout << "    TrailingMove     : " << trail << "\n";
    std::cout << "    FullClose        : " << full << "\n";
    std::cout << "    SwapAccrued      : " << swap_ev << "\n";
    std::cout << "  Active at end      : " << pm.activeCount() << "\n";
    std::cout << "  Realized PnL (USD) : " << std::setprecision(2)
              << pm.realizedTotal() << "\n\n";
    // Детали первых 10 событий
    std::cout << "--- First 10 events (detailed) ---\n";
    std::cout << std::fixed << std::setprecision(5);
    int shown = 0;
    for (std::size_t i = WINDOW; i < bars.size() && shown < 10; ++i) {
        std::vector<core::Bar> slice(bars.begin() + (i - WINDOW), bars.begin() + i + 1);
        auto mctx = ctx_agg.analyze({}, slice);
        auto sig  = pat_agg.analyze(slice, mctx);
        if (sig.detected) {
            auto r = val.validate(sig, mctx, slice.back(), acc);
            if (r.is_approved) {
                pm.openPosition(r.order, slice.back().spread, slice.back().timestamp);
            }
        }
        position::PositionAccountContext pac;
        pac.margin_level_pct = 5000.0;
        pac.now_ms           = slice.back().timestamp;
        auto evs = pm.onBar(slice.back(), pac);
        for (const auto& e : evs) {
            if (shown >= 10) break;
            ++shown;
            std::cout << "#" << shown
                      << "  bar=" << i
                      << "  " << event_name(e.type)
                      << "  id=" << e.position_id
                      << "  price=" << std::setprecision(5) << e.price;
            if (e.volume > 0)
                std::cout << "  vol=" << std::setprecision(2) << e.volume;
            if (e.new_sl > 0)
                std::cout << "  new_sl=" << std::setprecision(5) << e.new_sl;
            if (e.type == position::PositionEventType::FullClose ||
                e.type == position::PositionEventType::PartialClose)
                std::cout << "  net=$" << std::setprecision(2) << e.net_pnl;
            if (!e.reason.empty())
                std::cout << "  [" << e.reason << "]";
            std::cout << "\n";
            std::cout << std::setprecision(5);
        }
    }
    if (shown == 0) std::cout << "  (no events)\n";
    std::cout << "\nDone.\n";
    return 0;
}