// Smoke: EmergencyCloseHandler.
#include "position/EmergencyCloseHandler.h"
#include <iostream>
#include <iomanip>
using namespace spartak;
using position::EmergencyReason;
static constexpr int64_t DAY = 86'400'000LL;
static constexpr int64_t T0  = 1704067200000LL;
static const char* name(EmergencyReason r) {
    return position::to_string(r);
}
int main() {
    std::cout << "=== EmergencyCloseHandler smoke test ===\n\n";
    std::cout << std::fixed << std::setprecision(2);
    position::EmergencyConfig cfg;
    cfg.min_margin_level_pct = 1000.0;
    cfg.max_drawdown_pct     = 50.0;
    cfg.max_holding_days     = 30;
    position::EmergencyCloseHandler h(cfg);
    // --- T1: всё ок ---
    {
        position::EmergencyContext ctx;
        ctx.margin_level_pct = 5000.0;
        ctx.risk_money       = 100.0;
        ctx.current_pnl      = 30.0;   // прибыль
        ctx.open_time_ms     = T0;
        ctx.now_ms           = T0 + 5 * DAY;
        auto d = h.check(ctx);
        std::cout << "T1 healthy             : close=" << (d.should_close ? "Y" : "N")
                  << "  (expect N)  "
                  << (!d.should_close ? "OK" : "FAIL") << "\n";
    }
    // --- T2: margin call ---
    {
        position::EmergencyContext ctx;
        ctx.margin_level_pct = 500.0;   // < 1000
        ctx.risk_money       = 100.0;
        ctx.current_pnl      = 0.0;
        ctx.open_time_ms     = T0;
        ctx.now_ms           = T0 + DAY;
        auto d = h.check(ctx);
        std::cout << "T2 margin call         : close=" << (d.should_close ? "Y" : "N")
                  << "  reason=" << name(d.reason)
                  << "  value=" << d.value
                  << "  (expect Y/MarginCall/500.00)  "
                  << (d.should_close && d.reason == EmergencyReason::MarginCall ? "OK" : "FAIL") << "\n";
    }
    // --- T3: drawdown (60% от risk_money) ---
    {
        position::EmergencyContext ctx;
        ctx.margin_level_pct = 5000.0;
        ctx.risk_money       = 100.0;
        ctx.current_pnl      = -60.0;   // -60% от risk_money
        ctx.open_time_ms     = T0;
        ctx.now_ms           = T0 + DAY;
        auto d = h.check(ctx);
        std::cout << "T3 drawdown 60%        : close=" << (d.should_close ? "Y" : "N")
                  << "  reason=" << name(d.reason)
                  << "  value=" << d.value
                  << "  (expect Y/DrawdownLimit/60.00)  "
                  << (d.should_close && d.reason == EmergencyReason::DrawdownLimit ? "OK" : "FAIL") << "\n";
    }
    // --- T4: drawdown ровно на границе (50%) -> НЕ срабатывает ---
    {
        position::EmergencyContext ctx;
        ctx.margin_level_pct = 5000.0;
        ctx.risk_money       = 100.0;
        ctx.current_pnl      = -50.0;
        ctx.open_time_ms     = T0;
        ctx.now_ms           = T0 + DAY;
        auto d = h.check(ctx);
        std::cout << "T4 dd exact 50%        : close=" << (d.should_close ? "Y" : "N")
                  << "  (expect N — строго > 50)  "
                  << (!d.should_close ? "OK" : "FAIL") << "\n";
    }
    // --- T5: drawdown 51% -> срабатывает ---
    {
        position::EmergencyContext ctx;
        ctx.margin_level_pct = 5000.0;
        ctx.risk_money       = 100.0;
        ctx.current_pnl      = -51.0;
        ctx.open_time_ms     = T0;
        ctx.now_ms           = T0 + DAY;
        auto d = h.check(ctx);
        std::cout << "T5 dd 51%              : close=" << (d.should_close ? "Y" : "N")
                  << "  reason=" << name(d.reason)
                  << "  (expect Y/DrawdownLimit)  "
                  << (d.should_close && d.reason == EmergencyReason::DrawdownLimit ? "OK" : "FAIL") << "\n";
    }
    // --- T6: max holding ровно 30 дней -> НЕ срабатывает ---
    {
        position::EmergencyContext ctx;
        ctx.margin_level_pct = 5000.0;
        ctx.risk_money       = 100.0;
        ctx.current_pnl      = 0.0;
        ctx.open_time_ms     = T0;
        ctx.now_ms           = T0 + 30 * DAY;
        auto d = h.check(ctx);
        std::cout << "T6 holding exact 30d   : close=" << (d.should_close ? "Y" : "N")
                  << "  (expect N — строго > 30)  "
                  << (!d.should_close ? "OK" : "FAIL") << "\n";
    }
    // --- T7: max holding 31 день -> срабатывает ---
    {
        position::EmergencyContext ctx;
        ctx.margin_level_pct = 5000.0;
        ctx.risk_money       = 100.0;
        ctx.current_pnl      = 0.0;
        ctx.open_time_ms     = T0;
        ctx.now_ms           = T0 + 31 * DAY;
        auto d = h.check(ctx);
        std::cout << "T7 holding 31d         : close=" << (d.should_close ? "Y" : "N")
                  << "  reason=" << name(d.reason)
                  << "  value=" << d.value
                  << "  (expect Y/MaxHoldingTime/31.00)  "
                  << (d.should_close && d.reason == EmergencyReason::MaxHoldingTime ? "OK" : "FAIL") << "\n";
    }
    // --- T8: приоритет margin > drawdown ---
    {
        position::EmergencyContext ctx;
        ctx.margin_level_pct = 500.0;
        ctx.risk_money       = 100.0;
        ctx.current_pnl      = -80.0;
        ctx.open_time_ms     = T0;
        ctx.now_ms           = T0 + DAY;
        auto d = h.check(ctx);
        std::cout << "T8 priority margin>dd  : reason=" << name(d.reason)
                  << "  (expect MarginCall)  "
                  << (d.reason == EmergencyReason::MarginCall ? "OK" : "FAIL") << "\n";
    }
    // --- T9: drawdown disabled ---
    {
        position::EmergencyConfig cfg2;
        cfg2.drawdown_check_enabled = false;
        position::EmergencyCloseHandler h2(cfg2);
        position::EmergencyContext ctx;
        ctx.margin_level_pct = 5000.0;
        ctx.risk_money       = 100.0;
        ctx.current_pnl      = -90.0;    // 90% drawdown
        ctx.open_time_ms     = T0;
        ctx.now_ms           = T0 + DAY;
        auto d = h2.check(ctx);
        std::cout << "T9 dd disabled         : close=" << (d.should_close ? "Y" : "N")
                  << "  (expect N)  "
                  << (!d.should_close ? "OK" : "FAIL") << "\n";
    }
    // --- T10: pnl положительный (прибыль) -> не триггерит drawdown ---
    {
        position::EmergencyContext ctx;
        ctx.margin_level_pct = 5000.0;
        ctx.risk_money       = 100.0;
        ctx.current_pnl      = 250.0;    // прибыль, не drawdown
        ctx.open_time_ms     = T0;
        ctx.now_ms           = T0 + DAY;
        auto d = h.check(ctx);
        std::cout << "T10 profit (no dd)     : close=" << (d.should_close ? "Y" : "N")
                  << "  (expect N)  "
                  << (!d.should_close ? "OK" : "FAIL") << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}