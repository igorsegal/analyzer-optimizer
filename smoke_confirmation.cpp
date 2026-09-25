// Smoke: BarCloseConfirmationCounter.
#include "patterns/BarCloseConfirmationCounter.h"
#include <iostream>
#include <vector>
#include <cstdint>
using namespace spartak;
static core::Bar mk(int64_t t, double close) {
    return core::Bar{t, close - 0.001, close + 0.001, close - 0.002, close, 0, 0};
}
int main() {
    std::cout << "=== BarCloseConfirmationCounter smoke test ===\n\n";
    // --- Сценарий 1: 5 баров выше уровня 1.0000 ---
    {
        std::vector<core::Bar> bars = {
            mk(1, 1.0010), mk(2, 1.0015), mk(3, 1.0020),
            mk(4, 1.0018), mk(5, 1.0022),
        };
        auto n = patterns::BarCloseConfirmationCounter::countAbove(bars, 1.0000);
        std::cout << "T1: 5 bars all above 1.0  -> countAbove=" << n
                  << "  (expect 5)\n";
        bool ok = patterns::BarCloseConfirmationCounter::isConfirmedAbove(bars, 1.0, 3);
        std::cout << "    isConfirmedAbove(3)   -> " << (ok ? "YES" : "NO")
                  << "  (expect YES)\n";
        ok = patterns::BarCloseConfirmationCounter::isConfirmedAbove(bars, 1.0, 6);
        std::cout << "    isConfirmedAbove(6)   -> " << (ok ? "YES" : "NO")
                  << "  (expect NO)\n\n";
    }
    // --- Сценарий 2: 3 выше, 2 ниже ---
    {
        std::vector<core::Bar> bars = {
            mk(1, 0.9990), mk(2, 0.9995),
            mk(3, 1.0010), mk(4, 1.0020), mk(5, 1.0015),
        };
        auto n = patterns::BarCloseConfirmationCounter::countAbove(bars, 1.0000);
        std::cout << "T2: 2 below, 3 above 1.0  -> countAbove=" << n
                  << "  (expect 3)\n";
        bool ok = patterns::BarCloseConfirmationCounter::isConfirmedAbove(bars, 1.0, 3);
        std::cout << "    isConfirmedAbove(3)   -> " << (ok ? "YES" : "NO")
                  << "  (expect YES)\n\n";
    }
    // --- Сценарий 3: последний бар сломал последовательность ---
    {
        std::vector<core::Bar> bars = {
            mk(1, 1.0010), mk(2, 1.0020), mk(3, 1.0015),
            mk(4, 0.9999),
        };
        auto n = patterns::BarCloseConfirmationCounter::countAbove(bars, 1.0000);
        std::cout << "T3: last bar below 1.0    -> countAbove=" << n
                  << "  (expect 0)\n";
        bool ok = patterns::BarCloseConfirmationCounter::isConfirmedAbove(bars, 1.0, 1);
        std::cout << "    isConfirmedAbove(1)   -> " << (ok ? "YES" : "NO")
                  << "  (expect NO)\n\n";
    }
    // --- Сценарий 4: симметрия countBelow ---
    {
        std::vector<core::Bar> bars = {
            mk(1, 1.0010), mk(2, 0.9990), mk(3, 0.9985), mk(4, 0.9980),
        };
        auto n = patterns::BarCloseConfirmationCounter::countBelow(bars, 1.0000);
        std::cout << "T4: 1 above, 3 below 1.0  -> countBelow=" << n
                  << "  (expect 3)\n";
        bool ok = patterns::BarCloseConfirmationCounter::isConfirmedBelow(bars, 1.0, 3);
        std::cout << "    isConfirmedBelow(3)   -> " << (ok ? "YES" : "NO")
                  << "  (expect YES)\n\n";
    }
    // --- Сценарий 5: пустая история ---
    {
        std::vector<core::Bar> empty;
        auto n = patterns::BarCloseConfirmationCounter::countAbove(empty, 1.0);
        std::cout << "T5: empty vector          -> countAbove=" << n
                  << "  (expect 0)\n";
        bool ok = patterns::BarCloseConfirmationCounter::isConfirmedAbove(empty, 1.0, 1);
        std::cout << "    isConfirmedAbove(1)   -> " << (ok ? "YES" : "NO")
                  << "  (expect NO)\n\n";
    }
    // --- Сценарий 6: min_bars=0 всегда подтверждён ---
    {
        std::vector<core::Bar> bars = { mk(1, 0.9990) };
        bool ok = patterns::BarCloseConfirmationCounter::isConfirmedAbove(bars, 1.0, 0);
        std::cout << "T6: min_bars=0            -> isConfirmedAbove="
                  << (ok ? "YES" : "NO") << "  (expect YES)\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}