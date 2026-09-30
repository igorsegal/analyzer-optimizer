// Smoke: EngulfingDetector.
#include "patterns/EngulfingDetector.h"
#include <iostream>
#include <iomanip>
#include <vector>
using namespace spartak;
static core::Bar mk(int64_t t, double o, double h, double l, double c) {
    return core::Bar{t, o, h, l, c, 0, 0};
}
static void show(const char* name, const patterns::EngulfingSignal& s) {
    std::cout << name
              << "  detected=" << (s.detected ? "YES" : "NO");
    if (s.detected) {
        std::cout << "  side=" << (s.is_bullish ? "BULL" : "BEAR")
                  << "  conf=" << std::setprecision(3) << s.confidence
                  << "  trigger=" << s.trigger_price
                  << "  stop=" << s.suggested_stop;
    }
    std::cout << "\n";
}
int main() {
    std::cout << "=== EngulfingDetector smoke test ===\n\n";
    patterns::EngulfingDetector det;
    {
        auto p = mk(1, 1.0030, 1.0035, 1.0005, 1.0010);
        auto c = mk(2, 1.0005, 1.0040, 1.0000, 1.0035);
        show("T1 Bullish engulfing (ideal)    :", det.detect(p, c));
    }
    {
        auto p = mk(1, 1.0010, 1.0040, 1.0005, 1.0035);
        auto c = mk(2, 1.0035, 1.0040, 1.0000, 1.0005);
        show("T2 Bearish engulfing (ideal)    :", det.detect(p, c));
    }
    {
        auto p = mk(1, 1.0030, 1.0035, 1.0005, 1.0010);
        auto c = mk(2, 1.0015, 1.0045, 1.0010, 1.0040);
        show("T3 Not engulfing (open too high):", det.detect(p, c));
    }
    {
        auto p = mk(1, 1.0030, 1.0035, 1.0005, 1.0010);
        auto c = mk(2, 1.0008, 1.0015, 1.0005, 1.0012);
        show("T4 curr body too small          :", det.detect(p, c));
    }
    {
        auto p = mk(1, 1.0000, 1.0030, 0.9995, 1.0025);
        auto c = mk(2, 1.0005, 1.0040, 1.0000, 1.0035);
        show("T5 Both bullish (not engulfing) :", det.detect(p, c));
    }
    {
        std::vector<core::Bar> bars = {
            mk(1, 1.0030, 1.0035, 1.0005, 1.0010),
            mk(2, 1.0005, 1.0040, 1.0000, 1.0035),
        };
        show("T6 detectLast 2 bars            :", det.detectLast(bars));
    }
    {
        std::vector<core::Bar> empty;
        auto s1 = det.detectLast(empty);
        std::vector<core::Bar> one = { mk(1, 1.0010, 1.0020, 1.0000, 1.0015) };
        auto s2 = det.detectLast(one);
        std::cout << "T7 detectLast empty             : detected=" << (s1.detected ? "YES" : "NO") << "  (expect NO)\n";
        std::cout << "   detectLast 1 bar             : detected=" << (s2.detected ? "YES" : "NO") << "  (expect NO)\n";
    }
    {
        patterns::EngulfingConfig cfg;
        cfg.min_body_ratio_over_prev = 1.5;
        patterns::EngulfingDetector d2(cfg);
        auto p = mk(1, 1.0030, 1.0035, 1.0005, 1.0010);
        auto c = mk(2, 1.0005, 1.0040, 1.0000, 1.0035);
        show("T8 min_body_ratio=1.5 (edge)    :", d2.detect(p, c));
    }
    std::cout << "\nDone.\n";
    return 0;
}