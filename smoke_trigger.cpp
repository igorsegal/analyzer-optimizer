// Smoke: UO1Trigger.
#include "position/UO1Trigger.h"
#include <iostream>
#include <iomanip>
using namespace spartak;
using position::TriggerEvent;
static core::Bar mk(double h, double l) {
    return core::Bar{0, 1.0, h, l, 1.0, 0, 0};
}
static const char* name(TriggerEvent e) {
    return position::to_string(e);
}
int main() {
    std::cout << "=== UO1Trigger smoke test ===\n\n";
    position::UO1Trigger trg;
    {
        position::TriggerContext ctx;
        ctx.side = core::OrderSide::Buy;
        ctx.sl  = 1.0980;
        ctx.tp1 = 1.1030;
        ctx.tp2 = 1.1060;
        auto e = trg.check(mk(1.1010, 1.0990), ctx);
        std::cout << "T1 BUY inside           : " << name(e) << "  (expect None)  "
                  << (e == TriggerEvent::None ? "OK" : "FAIL") << "\n";
    }
    {
        position::TriggerContext ctx;
        ctx.side = core::OrderSide::Buy;
        ctx.sl  = 1.0980;
        ctx.tp1 = 1.1030;
        ctx.tp2 = 1.1060;
        auto e = trg.check(mk(1.1040, 1.0990), ctx);
        std::cout << "T2 BUY TP1 hit          : " << name(e) << "  (expect TP1_Hit)  "
                  << (e == TriggerEvent::TP1_Hit ? "OK" : "FAIL") << "\n";
    }
    {
        position::TriggerContext ctx;
        ctx.side = core::OrderSide::Buy;
        ctx.sl  = 1.0980;
        ctx.tp1 = 1.1030;
        ctx.tp2 = 1.1060;
        auto e = trg.check(mk(1.1070, 1.0990), ctx);
        std::cout << "T3 BUY TP2 hit          : " << name(e) << "  (expect TP2_Hit)  "
                  << (e == TriggerEvent::TP2_Hit ? "OK" : "FAIL") << "\n";
    }
    {
        position::TriggerContext ctx;
        ctx.side = core::OrderSide::Buy;
        ctx.sl  = 1.0980;
        ctx.tp1 = 1.1030;
        ctx.tp2 = 1.1060;
        auto e = trg.check(mk(1.1000, 1.0970), ctx);
        std::cout << "T4 BUY SL hit           : " << name(e) << "  (expect SL_Hit)  "
                  << (e == TriggerEvent::SL_Hit ? "OK" : "FAIL") << "\n";
    }
    {
        position::TriggerContext ctx;
        ctx.side = core::OrderSide::Buy;
        ctx.sl  = 1.0980;
        ctx.tp1 = 1.1030;
        ctx.tp2 = 1.1060;
        auto e = trg.check(mk(1.1040, 1.0970), ctx);
        std::cout << "T5 BUY SL+TP1 same bar  : " << name(e) << "  (expect SL_Hit)  "
                  << (e == TriggerEvent::SL_Hit ? "OK" : "FAIL") << "\n";
    }
    {
        position::TriggerContext ctx;
        ctx.side = core::OrderSide::Buy;
        ctx.sl  = 1.0980;
        ctx.tp1 = 1.1030;
        ctx.tp2 = 1.1060;
        ctx.tp1_hit = true;
        auto e = trg.check(mk(1.1070, 1.0990), ctx);
        std::cout << "T6 BUY TP1 hit, TP2 now : " << name(e) << "  (expect TP2_Hit)  "
                  << (e == TriggerEvent::TP2_Hit ? "OK" : "FAIL") << "\n";
    }
    {
        position::TriggerContext ctx;
        ctx.side = core::OrderSide::Buy;
        ctx.sl  = 1.0980;
        ctx.tp1 = 1.1030;
        ctx.tp2 = 1.1060;
        ctx.tp1_hit = true;
        auto e = trg.check(mk(1.1040, 1.0990), ctx);
        std::cout << "T7 BUY TP1 hit, no TP2  : " << name(e) << "  (expect None)  "
                  << (e == TriggerEvent::None ? "OK" : "FAIL") << "\n";
    }
    {
        position::TriggerContext ctx;
        ctx.side = core::OrderSide::Sell;
        ctx.sl  = 1.1020;
        ctx.tp1 = 1.0970;
        ctx.tp2 = 1.0940;
        auto e = trg.check(mk(1.1010, 1.0990), ctx);
        std::cout << "T8 SELL inside          : " << name(e) << "  (expect None)  "
                  << (e == TriggerEvent::None ? "OK" : "FAIL") << "\n";
    }
    {
        position::TriggerContext ctx;
        ctx.side = core::OrderSide::Sell;
        ctx.sl  = 1.1020;
        ctx.tp1 = 1.0970;
        ctx.tp2 = 1.0940;
        auto e = trg.check(mk(1.1000, 1.0960), ctx);
        std::cout << "T9 SELL TP1 hit         : " << name(e) << "  (expect TP1_Hit)  "
                  << (e == TriggerEvent::TP1_Hit ? "OK" : "FAIL") << "\n";
    }
    {
        position::TriggerContext ctx;
        ctx.side = core::OrderSide::Sell;
        ctx.sl  = 1.1020;
        ctx.tp1 = 1.0970;
        ctx.tp2 = 1.0940;
        auto e = trg.check(mk(1.1000, 1.0930), ctx);
        std::cout << "T10 SELL TP2 hit        : " << name(e) << "  (expect TP2_Hit)  "
                  << (e == TriggerEvent::TP2_Hit ? "OK" : "FAIL") << "\n";
    }
    {
        position::TriggerContext ctx;
        ctx.side = core::OrderSide::Sell;
        ctx.sl  = 1.1020;
        ctx.tp1 = 1.0970;
        ctx.tp2 = 1.0940;
        auto e = trg.check(mk(1.1030, 1.1000), ctx);
        std::cout << "T11 SELL SL hit         : " << name(e) << "  (expect SL_Hit)  "
                  << (e == TriggerEvent::SL_Hit ? "OK" : "FAIL") << "\n";
    }
    {
        auto e = trg.checkRaw(mk(1.1040, 1.0990), core::OrderSide::Buy, 1.0980, 1.1030, 1.1060);
        std::cout << "T12 checkRaw BUY TP1    : " << name(e) << "  (expect TP1_Hit)  "
                  << (e == TriggerEvent::TP1_Hit ? "OK" : "FAIL") << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}