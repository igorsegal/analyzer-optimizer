// Smoke: InsideBarDetector.
#include "patterns/InsideBarDetector.h"
#include <iostream>
#include <iomanip>
#include <vector>
using namespace spartak;
static core::Bar mk(int64_t t, double o, double h, double l, double c) {
    return core::Bar{t, o, h, l, c, 0, 0};
}
int main() {
    std::cout << "=== InsideBarDetector smoke test ===\n\n";
    patterns::InsideBarDetector det;
    // T1: классический inside bar
    {
        auto m = mk(1, 1.0010, 1.0050, 1.0000, 1.0040);
        auto i = mk(2, 1.0020, 1.0030, 1.0015, 1.0025);
        auto s = det.detect(m, i);
        std::cout << "T1: classic inside bar     : detected=" << (s.detected ? "YES" : "NO")
                  << "  ratio=" << std::setprecision(3) << s.range_ratio
                  << "  conf=" << s.confidence << "  (expect YES)\n";
    }
    // T2: не внутри (high выше материнского)
    {
        auto m = mk(1, 1.0010, 1.0050, 1.0000, 1.0040);
        auto i = mk(2, 1.0020, 1.0060, 1.0015, 1.0025);
        auto s = det.detect(m, i);
        std::cout << "T2: high above mother      : detected=" << (s.detected ? "YES" : "NO")
                  << "  (expect NO)\n";
    }
    // T3: не внутри (low ниже материнского)
    {
        auto m = mk(1, 1.0010, 1.0050, 1.0000, 1.0040);
        auto i = mk(2, 1.0020, 1.0030, 0.9995, 1.0025);
        auto s = det.detect(m, i);
        std::cout << "T3: low below mother       : detected=" << (s.detected ? "YES" : "NO")
                  << "  (expect NO)\n";
    }
    // T4: identical bars
    {
        auto m = mk(1, 1.0010, 1.0050, 1.0000, 1.0040);
        auto i = mk(2, 1.0010, 1.0050, 1.0000, 1.0040);
        auto s = det.detect(m, i);
        std::cout << "T4: identical bars         : detected=" << (s.detected ? "YES" : "NO")
                  << "  ratio=" << std::setprecision(3) << s.range_ratio
                  << "  conf=" << s.confidence << "  (expect YES, conf=0)\n";
    }
    // T5: очень узкий inside
    {
        auto m = mk(1, 1.0010, 1.0100, 1.0000, 1.0050);
        auto i = mk(2, 1.0050, 1.0055, 1.0048, 1.0052);
        auto s = det.detect(m, i);
        std::cout << "T5: tight inside (5%)      : detected=" << (s.detected ? "YES" : "NO")
                  << "  ratio=" << std::setprecision(3) << s.range_ratio
                  << "  conf=" << s.confidence << "  (expect YES, high conf)\n";
    }
    // T6: detectLast
    {
        std::vector<core::Bar> bars = {
            mk(1, 1.0010, 1.0050, 1.0000, 1.0040),
            mk(2, 1.0020, 1.0030, 1.0015, 1.0025),
        };
        auto s = det.detectLast(bars);
        std::cout << "T6: detectLast on 2 bars   : detected=" << (s.detected ? "YES" : "NO")
                  << "  (expect YES)\n";
    }
    // T7: detectLast на пустом векторе
    {
        std::vector<core::Bar> empty;
        auto s = det.detectLast(empty);
        std::cout << "T7: detectLast empty       : detected=" << (s.detected ? "YES" : "NO")
                  << "  (expect NO)\n";
    }
    // T8: минимальное сжатие (min_range_ratio)
    {
        patterns::InsideBarConfig cfg;
        cfg.min_range_ratio = 0.40;
        patterns::InsideBarDetector d2(cfg);
        auto m = mk(1, 1.0010, 1.0050, 1.0000, 1.0040);
        auto i = mk(2, 1.0020, 1.0030, 1.0015, 1.0025);
        auto s = d2.detect(m, i);
        std::cout << "T8: min_range_ratio=0.40   : detected=" << (s.detected ? "YES" : "NO")
                  << "  (expect NO, ratio=0.30 < 0.40)\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}