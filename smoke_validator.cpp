// Smoke: SignalValidator — ПОЛНЫЙ end-to-end pipeline.
// Bar -> context -> patterns -> validation -> ValidatedOrderRequest
#include "data/BarStream.h"
#include "data/DataSanitizer.h"
#include "context/ContextAggregator.h"
#include "patterns/PatternAggregator.h"
#include "validation/SignalValidator.h"
#include <iostream>
#include <iomanip>
#include <vector>
using namespace spartak;
static const char* reject_name(core::RejectReason r) {
    return core::to_string(r);
}
static const char* side_name(core::OrderSide s) {
    return (s == core::OrderSide::Buy) ? "BUY" : "SELL";
}
int main(int argc, char** argv) {
    const std::string path = (argc > 1)
        ? argv[1]
        : "D:/AHexaTrader/1DataFiles/raw/EURUSD/EURUSD_M5_real.bin";
    std::cout << "=== SignalValidator END-TO-END smoke test ===\n\n";
    data::BarStream stream(path);
    if (!stream.is_ok()) { std::cout << "Stream open FAILED\n"; return 1; }
    data::DataSanitizer sanitizer;
    (void)sanitizer.run(stream);
    constexpr int N = 500;
    std::vector<core::Bar> bars;
    bars.reserve(N);
    core::Bar b;
    while (bars.size() < N && stream.next(b)) bars.push_back(b);
    std::cout << "Bars loaded : " << bars.size() << "\n\n";
    // --- Конфиги ---
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
    val_cfg.spread.max_points = 30;    // мягкий лимит
    val_cfg.counter_trend.allow_counter_trend_on_pu = true;
    val_cfg.counter_trend.point = 0.00001;
    val_cfg.money.risk_percent  = 1.0;
    val_cfg.money.contract_size = 100'000.0;
    val_cfg.money.point         = 0.00001;
    val_cfg.lot_rounder.min_lot  = 0.01;
    val_cfg.lot_rounder.max_lot  = 1.0;
    val_cfg.lot_rounder.lot_step = 0.01;
    val_cfg.margin.contract_size = 100'000.0;
    val_cfg.session.enabled = false;   // выключено для теста
    context::ContextAggregator ctx_agg(ctx_cfg);
    patterns::PatternAggregator pat_agg(pat_cfg);
    validation::SignalValidator val(val_cfg);
    validation::AccountSnapshot acc;
    acc.balance              = 10'000.0;
    acc.equity               = 10'000.0;
    acc.free_margin          = 10'000.0;
    acc.margin_used          = 0.0;
    acc.leverage             = 500.0;
    acc.commission_per_lot   = 5.0;
    acc.min_margin_level_pct = 5'000.0;
    constexpr size_t WINDOW = 100;
    size_t signals_total    = 0;
    size_t approved         = 0;
    size_t rejected_spread  = 0;
    size_t rejected_trend   = 0;
    size_t rejected_margin  = 0;
    size_t rejected_other   = 0;
    double total_risk_usd   = 0.0;
    for (size_t i = WINDOW; i < bars.size(); ++i) {
        std::vector<core::Bar> slice(bars.begin() + (i - WINDOW), bars.begin() + i + 1);
        auto mctx = ctx_agg.analyze({}, slice);
        auto sig  = pat_agg.analyze(slice, mctx);
        if (!sig.detected) continue;
        ++signals_total;
        auto r = val.validate(sig, mctx, slice.back(), acc);
        if (r.is_approved) {
            ++approved;
            total_risk_usd += r.risk_amount;
        } else {
            switch (r.reason) {
                case core::RejectReason::SpreadTooHigh:  ++rejected_spread; break;
                case core::RejectReason::TrendConflict:  ++rejected_trend;  break;
                case core::RejectReason::MarginCall:
                case core::RejectReason::MarginTooLow:   ++rejected_margin; break;
                default:                                  ++rejected_other;  break;
            }
        }
    }
    std::cout << "--- Validation summary ---\n";
    std::cout << "  Total signals       : " << signals_total << "\n";
    std::cout << "  Approved            : " << approved << "\n";
    std::cout << "  Rejected (spread)   : " << rejected_spread << "\n";
    std::cout << "  Rejected (trend)    : " << rejected_trend << "\n";
    std::cout << "  Rejected (margin)   : " << rejected_margin << "\n";
    std::cout << "  Rejected (other)    : " << rejected_other << "\n";
    std::cout << "  Sum of risk (USD)   : " << std::setprecision(2) << total_risk_usd << "\n\n";
    // --- Детали первых 5 одобренных ---
    std::cout << "--- First 5 APPROVED orders ---\n";
    std::cout << std::fixed << std::setprecision(5);
    int shown = 0;
    for (size_t i = WINDOW; i < bars.size() && shown < 5; ++i) {
        std::vector<core::Bar> slice(bars.begin() + (i - WINDOW), bars.begin() + i + 1);
        auto mctx = ctx_agg.analyze({}, slice);
        auto sig  = pat_agg.analyze(slice, mctx);
        if (!sig.detected) continue;
        auto r = val.validate(sig, mctx, slice.back(), acc);
        if (!r.is_approved) continue;
        ++shown;
        std::cout << "#" << shown
                  << "  bar=" << i
                  << "  " << side_name(r.order.side)
                  << "  vol=" << std::setprecision(2) << r.order.volume
                  << "  entry=" << std::setprecision(5) << r.order.entry_price
                  << "  sl=" << r.order.stop_loss
                  << "  tp1=" << r.order.take_profit_1
                  << "  tp2=" << r.order.take_profit_2
                  << "  risk=$" << std::setprecision(2) << r.risk_amount
                  << "  R:R=" << r.rr_ratio
                  << "\n";
        std::cout << std::setprecision(5);
    }
    if (shown == 0) std::cout << "  (none)\n";
    std::cout << "\nDone.\n";
    return 0;
}