// Smoke: PatternAggregator — финальный тест слоя patterns/.
#include "data/BarStream.h"
#include "data/DataSanitizer.h"
#include "context/ContextAggregator.h"
#include "patterns/PatternAggregator.h"
#include <iostream>
#include <iomanip>
#include <vector>
using namespace spartak;
static const char* pattern_name(core::PatternType t) {
    switch (t) {
        case core::PatternType::FalseBreakout:   return "FalseBreakout";
        case core::PatternType::Consolidation:   return "Consolidation";
        case core::PatternType::ImpulseBreakout: return "ImpulseBreakout";
        default:                                 return "None";
    }
}
static const char* side_name(core::OrderSide s) {
    return (s == core::OrderSide::Buy) ? "BUY" : "SELL";
}
int main(int argc, char** argv) {
    const std::string path = (argc > 1)
        ? argv[1]
        : "D:/AHexaTrader/1DataFiles/raw/EURUSD/EURUSD_M5_real.bin";
    std::cout << "=== PatternAggregator smoke test (full patterns layer) ===\n\n";
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
    context::ContextAggregatorConfig ctx_cfg;
    ctx_cfg.point              = 0.00001;
    ctx_cfg.fractal_radius     = 2;
    ctx_cfg.max_zones_per_tf   = 5;
    ctx_cfg.use_same_tf_for_both = true;
    context::ContextAggregator ctx_agg(ctx_cfg);
    patterns::PatternAggregatorConfig pat_cfg;
    pat_cfg.point = 0.00001;
    pat_cfg.volume.min_volume  = 0;
    pat_cfg.volume.avg_ratio   = 0.0;
    pat_cfg.volume.avg_period  = 20;
    pat_cfg.require_trend_for_consolidation = false;
    patterns::PatternAggregator pat_agg(pat_cfg);
    size_t detected_fb   = 0;
    size_t detected_ib   = 0;
    size_t detected_cs   = 0;
    size_t buy_count     = 0;
    size_t sell_count    = 0;
    constexpr size_t WINDOW = 100;
    for (size_t i = WINDOW; i < bars.size(); ++i) {
        std::vector<core::Bar> slice(bars.begin() + (i - WINDOW), bars.begin() + i + 1);
        auto mctx = ctx_agg.analyze({}, slice);
        auto sig  = pat_agg.analyze(slice, mctx);
        if (sig.detected) {
            switch (sig.type) {
                case core::PatternType::FalseBreakout:   ++detected_fb; break;
                case core::PatternType::ImpulseBreakout: ++detected_ib; break;
                case core::PatternType::Consolidation:   ++detected_cs; break;
                default: break;
            }
            if (sig.side == core::OrderSide::Buy) ++buy_count;
            else                                  ++sell_count;
        }
    }
    std::cout << "--- Rolling scan over " << (bars.size() - WINDOW) << " bars ---\n";
    std::cout << "  FalseBreakout   : " << detected_fb << "\n";
    std::cout << "  ImpulseBreakout : " << detected_ib << "\n";
    std::cout << "  Consolidation   : " << detected_cs << "\n";
    std::cout << "  Total signals   : " << (detected_fb + detected_ib + detected_cs) << "\n";
    std::cout << "    BUY           : " << buy_count  << "\n";
    std::cout << "    SELL          : " << sell_count << "\n\n";
    std::cout << "--- First 5 signals (detailed) ---\n";
    std::cout << std::fixed << std::setprecision(5);
    int shown = 0;
    for (size_t i = WINDOW; i < bars.size() && shown < 5; ++i) {
        std::vector<core::Bar> slice(bars.begin() + (i - WINDOW), bars.begin() + i + 1);
        auto mctx = ctx_agg.analyze({}, slice);
        auto sig  = pat_agg.analyze(slice, mctx);
        if (!sig.detected) continue;
        ++shown;
        std::cout << "#" << shown
                  << "  bar=" << i
                  << "  " << side_name(sig.side)
                  << "  " << pattern_name(sig.type)
                  << "  trigger=" << sig.trigger_price
                  << "  level=" << sig.level
                  << "  stop=" << sig.suggested_stop
                  << "  conf=" << std::setprecision(3) << sig.confidence
                  << std::setprecision(5)
                  << "\n";
    }
    if (shown == 0) std::cout << "  (no signals detected)\n";
    std::cout << "\nDone.\n";
    return 0;
}