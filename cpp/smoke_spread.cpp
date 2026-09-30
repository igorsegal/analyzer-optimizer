// Smoke: SpreadFilter.
#include "validation/SpreadFilter.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdint>
using namespace spartak;
static core::Bar mk(int64_t t, int32_t spread) {
    return core::Bar{t, 1.0, 1.0001, 0.9999, 1.0, 0, spread};
}
int main() {
    std::cout << "=== SpreadFilter smoke test ===\n\n";
    // --- Абсолютный ---
    {
        validation::SpreadFilterConfig cfg;
        cfg.max_points = 20;
        validation::SpreadFilter f(cfg);
        std::cout << "--- Absolute filter (max=20) ---\n";
        std::cout << "  spread=5  : " << (f.pass(5)  ? "PASS" : "REJECT") << " (expect PASS)\n";
        std::cout << "  spread=20 : " << (f.pass(20) ? "PASS" : "REJECT") << " (expect PASS)\n";
        std::cout << "  spread=21 : " << (f.pass(21) ? "PASS" : "REJECT") << " (expect REJECT)\n";
        std::cout << "  spread=99 : " << (f.pass(99) ? "PASS" : "REJECT") << " (expect REJECT)\n\n";
    }
    // --- Относительный ---
    {
        validation::SpreadFilterConfig cfg;
        cfg.max_avg_ratio = 1.5;   // спред ≤ 1.5 * среднего
        cfg.avg_period    = 10;
        validation::SpreadFilter f(cfg);
        // история со средним спредом = 10
        std::vector<core::Bar> hist;
        for (int i = 0; i < 10; ++i) hist.push_back(mk(i, 10));
        std::cout << "--- Relative filter (ratio=1.5, avg=10) ---\n";
        std::cout << "  spread=8   : " << (f.passWithAverage(8,  hist) ? "PASS" : "REJECT") << " (expect PASS)\n";
        std::cout << "  spread=15  : " << (f.passWithAverage(15, hist) ? "PASS" : "REJECT") << " (expect PASS)\n";
        std::cout << "  spread=16  : " << (f.passWithAverage(16, hist) ? "PASS" : "REJECT") << " (expect REJECT)\n";
        std::cout << "  spread=30  : " << (f.passWithAverage(30, hist) ? "PASS" : "REJECT") << " (expect REJECT)\n\n";
    }
    // --- Комбинированный ---
    {
        validation::SpreadFilterConfig cfg;
        cfg.max_points    = 15;
        cfg.max_avg_ratio = 1.5;
        cfg.avg_period    = 10;
        validation::SpreadFilter f(cfg);
        std::vector<core::Bar> hist;
        for (int i = 0; i < 10; ++i) hist.push_back(mk(i, 10));
        std::cout << "--- Combined (max=15, ratio=1.5, avg=10) ---\n";
        std::cout << "  spread=5   : " << (f.passAll(5,  hist) ? "PASS" : "REJECT") << " (expect PASS)\n";
        std::cout << "  spread=14  : " << (f.passAll(14, hist) ? "PASS" : "REJECT") << " (expect PASS)\n";
        std::cout << "  spread=16  : " << (f.passAll(16, hist) ? "PASS" : "REJECT") << " (expect REJECT, above abs)\n";
        std::cout << "  spread=15  : " << (f.passAll(15, hist) ? "PASS" : "REJECT") << " (expect PASS, at abs limit and <=1.5*avg)\n\n";
    }
    // --- Недостаток истории: fallback PASS ---
    {
        validation::SpreadFilterConfig cfg;
        cfg.max_avg_ratio = 1.5;
        cfg.avg_period    = 50;
        validation::SpreadFilter f(cfg);
        std::vector<core::Bar> tiny;
        for (int i = 0; i < 5; ++i) tiny.push_back(mk(i, 10));
        std::cout << "--- Not enough history ---\n";
        std::cout << "  spread=999 : " << (f.passWithAverage(999, tiny) ? "PASS" : "REJECT")
                  << " (expect PASS fallback)\n";
    }
    // --- Реальный средний по истории ---
    {
        validation::SpreadFilterConfig cfg;
        cfg.avg_period = 5;
        validation::SpreadFilter f(cfg);
        std::vector<core::Bar> hist = {
            mk(0, 2), mk(1, 4), mk(2, 6), mk(3, 8), mk(4, 10)
        };
        const double avg = f.averageSpread(hist);
        std::cout << "\n--- Average spread (5 bars: 2,4,6,8,10) ---\n";
        std::cout << "  avg = " << std::fixed << std::setprecision(2) << avg
                  << "  (expect 6.00)\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}