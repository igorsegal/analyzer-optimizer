// Smoke: CounterTrendPUChecker.
#include "validation/CounterTrendPUChecker.h"
#include <iostream>
#include <iomanip>
#include <vector>
using namespace spartak;
static core::PriceZone pu_zone(double level, double half_width = 0.0001) {
    core::PriceZone z;
    z.type           = core::LevelType::IntermediateLevel;
    z.formation_time = 0;
    z.price_level    = level;
    z.zone_top       = level + half_width;
    z.zone_bottom    = level - half_width;
    z.is_active      = true;
    return z;
}
static core::PriceZone lu_zone(double level, double half_width = 0.0001) {
    core::PriceZone z;
    z.type           = core::LevelType::LocalLevel;
    z.formation_time = 0;
    z.price_level    = level;
    z.zone_top       = level + half_width;
    z.zone_bottom    = level - half_width;
    z.is_active      = true;
    return z;
}
static void show(const char* name, const validation::CounterTrendResult& r) {
    std::cout << name
              << "  allowed=" << (r.allowed ? "YES" : "NO")
              << "  is_counter=" << (r.is_counter ? "Y" : "N")
              << "  pu_excuse=" << (r.pu_excuse ? "Y" : "N")
              << "\n";
}
int main() {
    std::cout << "=== CounterTrendPUChecker smoke test ===\n\n";
    validation::CounterTrendConfig cfg;
    cfg.point = 0.00001;
    cfg.pu_match_epsilon_points = 2.0;
    cfg.allow_counter_trend_on_pu = true;
    validation::CounterTrendPUChecker chk(cfg);
    using core::OrderSide;
    using core::TrendDirection;
    // --- T1: BUY по тренду Bullish ---
    {
        core::MarketContext ctx;
        ctx.dominant_trend = TrendDirection::Bullish;
        auto r = chk.check(OrderSide::Buy, TrendDirection::Bullish, 1.1000, ctx);
        show("T1 BUY with BULL        :", r);
    }
    // --- T2: SELL по тренду Bearish ---
    {
        core::MarketContext ctx;
        ctx.dominant_trend = TrendDirection::Bearish;
        auto r = chk.check(OrderSide::Sell, TrendDirection::Bearish, 1.1000, ctx);
        show("T2 SELL with BEAR       :", r);
    }
    // --- T3: тренд Undefined ---
    {
        core::MarketContext ctx;
        ctx.dominant_trend = TrendDirection::Undefined;
        auto r = chk.check(OrderSide::Buy, TrendDirection::Undefined, 1.1000, ctx);
        show("T3 BUY with UNDEF       :", r);
    }
    // --- T4: BUY против Bearish, БЕЗ ПУ -> REJECT ---
    {
        core::MarketContext ctx;
        ctx.dominant_trend = TrendDirection::Bearish;
        ctx.active_zones = { lu_zone(1.1000) };   // только ЛУ
        auto r = chk.check(OrderSide::Buy, TrendDirection::Bearish, 1.1000, ctx);
        show("T4 BUY vs BEAR, no PU   :", r);
    }
    // --- T5: BUY против Bearish, level НА ПУ -> allowed ---
    {
        core::MarketContext ctx;
        ctx.dominant_trend = TrendDirection::Bearish;
        ctx.active_zones = { pu_zone(1.1000) };
        auto r = chk.check(OrderSide::Buy, TrendDirection::Bearish, 1.1000, ctx);
        show("T5 BUY vs BEAR, on PU   :", r);
    }
    // --- T6: level в зоне ПУ (не точно на цене) ---
    {
        core::MarketContext ctx;
        ctx.dominant_trend = TrendDirection::Bearish;
        ctx.active_zones = { pu_zone(1.1000, 0.0005) };   // широкая зона
        auto r = chk.check(OrderSide::Buy, TrendDirection::Bearish, 1.1003, ctx);
        show("T6 BUY vs BEAR, in zone :", r);
    }
    // --- T7: SELL против Bullish, БЕЗ ПУ -> REJECT ---
    {
        core::MarketContext ctx;
        ctx.dominant_trend = TrendDirection::Bullish;
        ctx.active_zones = { lu_zone(1.1000) };
        auto r = chk.check(OrderSide::Sell, TrendDirection::Bullish, 1.1000, ctx);
        show("T7 SELL vs BULL, no PU  :", r);
    }
    // --- T8: SELL против Bullish, level НА ПУ -> allowed ---
    {
        core::MarketContext ctx;
        ctx.dominant_trend = TrendDirection::Bullish;
        ctx.active_zones = { pu_zone(1.1000) };
        auto r = chk.check(OrderSide::Sell, TrendDirection::Bullish, 1.1000, ctx);
        show("T8 SELL vs BULL, on PU  :", r);
    }
    // --- T9: allow_counter_trend_on_pu = false -> REJECT везде ---
    {
        validation::CounterTrendConfig cfg2;
        cfg2.point = 0.00001;
        cfg2.allow_counter_trend_on_pu = false;
        validation::CounterTrendPUChecker chk2(cfg2);
        core::MarketContext ctx;
        ctx.dominant_trend = TrendDirection::Bearish;
        ctx.active_zones = { pu_zone(1.1000) };
        auto r = chk2.check(OrderSide::Buy, TrendDirection::Bearish, 1.1000, ctx);
        show("T9 flag=false, on PU    :", r);
    }
    // --- T10: level далеко от ПУ -> REJECT ---
    {
        core::MarketContext ctx;
        ctx.dominant_trend = TrendDirection::Bearish;
        ctx.active_zones = { pu_zone(1.2000) };   // ПУ далеко от 1.1000
        auto r = chk.check(OrderSide::Buy, TrendDirection::Bearish, 1.1000, ctx);
        show("T10 BUY, PU far away    :", r);
    }
    std::cout << "\nDone.\n";
    return 0;
}