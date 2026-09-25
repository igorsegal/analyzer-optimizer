// Smoke: ATRImpulseBreakoutDetector.
#include "patterns/ATRImpulseBreakoutDetector.h"
#include <iostream>
#include <iomanip>
#include <vector>
using namespace spartak;
static core::Bar mk(int64_t t, double o, double h, double l, double c) {
    return core::Bar{t, o, h, l, c, 0, 0};
}
static void show(const char* name, const patterns::ImpulseSignal& s) {
    std::cout << name
              << "  detected=" << (s.detected ? "YES" : "NO");
    if (s.detected) {
        std::cout << "  side=" << (s.is_bullish ? "BULL" : "BEAR")
                  << "  conf=" << std::setprecision(3) << s.confidence
                  << "  trigger=" << s.trigger_price
                  << "  stop=" << s.suggested_stop
                  << "  level=" << s.level;
    }
    std::cout << "\n";
}
int main() {
    std::cout << "=== ATRImpulseBreakoutDetector smoke test ===\n\n";
    patterns::ATRImpulseBreakoutDetector det;
    std::vector<core::Bar> base;
    for (int i = 0; i < 20; ++i) {
        double c = 1.0000 + i * 0.00005;
        base.push_back(mk(i, c, c + 0.0010, c - 0.0010, c + 0.0001));
    }
    {
        auto bars = base;
        bars.push_back(mk(100, 1.0005, 1.0042, 1.0002, 1.0040));
        show("T1 Impulse BUY breakout     :", det.detect(bars, 1.0030));
    }
    {
        auto bars = base;
        bars.push_back(mk(100, 1.0040, 1.0042, 1.0002, 1.0005));
        show("T2 Impulse SELL breakout    :", det.detect(bars, 1.0020));
    }
    {
        auto bars = base;
        bars.push_back(mk(100, 1.0025, 1.0042, 1.0002, 1.0035));
        show("T3 Small body (not impulse) :", det.detect(bars, 1.0030));
    }
    {
        auto bars = base;
        bars.push_back(mk(100, 1.0030, 1.0035, 1.0028, 1.0034));
        show("T4 Range too small vs ATR   :", det.detect(bars, 1.0025));
    }
    {
        auto bars = base;
        bars.push_back(mk(100, 1.0005, 1.0042, 1.0002, 1.0040));
        show("T5 Close not above level    :", det.detect(bars, 1.0050));
    }
    {
        auto bars = base;
        bars.push_back(mk(100, 1.0035, 1.0100, 1.0002, 1.0035));
        show("T6 Bullish but low close_pos:", det.detect(bars, 1.0030));
    }
    {
        std::vector<core::Bar> tiny;
        tiny.push_back(mk(1, 1.0000, 1.0010, 0.9990, 1.0005));
        tiny.push_back(mk(2, 1.0005, 1.0020, 1.0000, 1.0015));
        show("T7 Not enough history       :", det.detect(tiny, 1.0000));
    }
    std::cout << "\nDone.\n";
    return 0;
}