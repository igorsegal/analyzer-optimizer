// Smoke: ContextAggregator — финальный тест слоя context/.
// Прогоняет всю цепочку на реальном M5 и показывает готовый MarketContext.
#include "data/BarStream.h"
#include "data/DataSanitizer.h"
#include "context/ContextAggregator.h"
#include <iostream>
#include <iomanip>
#include <vector>
using namespace spartak;
static const char* trend_name(core::TrendDirection t) {
    switch (t) {
        case core::TrendDirection::Bullish: return "Bullish";
        case core::TrendDirection::Bearish: return "Bearish";
        default:                            return "Undefined";
    }
}
int main(int argc, char** argv) {
    const std::string path = (argc > 1)
        ? argv[1]
        : "D:/AHexaTrader/1DataFiles/raw/EURUSD/EURUSD_M5_real.bin";
    std::cout << "=== ContextAggregator smoke test (full pipeline) ===\n\n";
    data::BarStream stream(path);
    if (!stream.is_ok()) { std::cout << "Stream open FAILED\n"; return 1; }
    data::DataSanitizer sanitizer;
    (void)sanitizer.run(stream);
    constexpr int N = 2000;
    std::vector<core::Bar> bars;
    bars.reserve(N);
    core::Bar b;
    while (bars.size() < N && stream.next(b)) bars.push_back(b);
    // Конфиг: PU/LU считаем на одних и тех же барах (use_same_tf_for_both=true)
    context::ContextAggregatorConfig cfg;
    cfg.point                  = 0.00001;
    cfg.fractal_radius         = 2;
    cfg.shadow_confirm_window  = 5;
    cfg.shadow_tolerance_points= 2;
    cfg.pu_offset_points       = 15;
    cfg.lu_offset_points       = 10;
    cfg.max_zones_per_tf       = 5;
    cfg.trend_lookback         = 3;
    cfg.max_level_age_ms       = 7LL * 86'400'000LL;
    cfg.level_break_buffer     = 30;
    cfg.use_same_tf_for_both   = true;
    context::ContextAggregator agg(cfg);
    auto ctx = agg.analyze(/*older*/ {}, /*younger*/ bars);
    std::cout << "Bars processed       : " << bars.size() << "\n\n";
    std::cout << "--- Trend analysis ---\n";
    std::cout << "  Daily trend        : " << trend_name(ctx.daily_trend)  << "\n";
    std::cout << "  Hourly trend       : " << trend_name(ctx.hourly_trend) << "\n";
    std::cout << "  Dominant trend     : " << trend_name(ctx.dominant_trend) << "\n";
    std::cout << "  HH/HL structure    : " << (ctx.has_hh_hl_structure ? "YES" : "NO") << "\n";
    std::cout << "  Trend conflict     : " << (ctx.trend_conflict ? "YES" : "NO") << "\n\n";
    // --- Считаем активные/неактивные зоны ---
    int pu_count = 0, lu_count = 0;
    int pu_active = 0, lu_active = 0;
    for (const auto& z : ctx.active_zones) {
        if (z.type == core::LevelType::IntermediateLevel) {
            ++pu_count;
            if (z.is_active) ++pu_active;
        } else {
            ++lu_count;
            if (z.is_active) ++lu_active;
        }
    }
    std::cout << "--- Zones ---\n";
    std::cout << "  PU total   : " << pu_count << "  (active: " << pu_active << ")\n";
    std::cout << "  LU total   : " << lu_count << "  (active: " << lu_active << ")\n\n";
    // --- Печать первых 5 активных зон ---
    std::cout << "--- Active zones (first 5) ---\n";
    std::cout << std::fixed << std::setprecision(5);
    int shown = 0;
    for (const auto& z : ctx.active_zones) {
        if (!z.is_active) continue;
        if (shown++ >= 5) break;
        std::cout << "  " << (z.type == core::LevelType::IntermediateLevel ? "PU" : "LU")
                  << "  level=" << z.price_level
                  << "  zone=[" << z.zone_bottom << " .. " << z.zone_top << "]\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}