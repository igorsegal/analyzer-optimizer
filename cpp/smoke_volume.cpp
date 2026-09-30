// Smoke: TickVolumeFilter.
#include "patterns/TickVolumeFilter.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdint>
using namespace spartak;
static core::Bar mk(int64_t t, int64_t vol, double c = 1.0) {
    return core::Bar{t, c, c + 0.001, c - 0.001, c, vol, 0};
}
int main() {
    std::cout << "=== TickVolumeFilter smoke test ===\n\n";
    // ---------- Тест 1: абсолютный порог ----------
    {
        patterns::TickVolumeConfig cfg;
        cfg.min_volume = 500;
        patterns::TickVolumeFilter f(cfg);
        std::cout << "--- Absolute filter (min=500) ---\n";
        auto b1 = mk(1, 400);
        auto b2 = mk(2, 500);
        auto b3 = mk(3, 700);
        std::cout << "  vol=400 : " << (f.pass(b1) ? "PASS" : "REJECT") << " (expect REJECT)\n";
        std::cout << "  vol=500 : " << (f.pass(b2) ? "PASS" : "REJECT") << " (expect PASS)\n";
        std::cout << "  vol=700 : " << (f.pass(b3) ? "PASS" : "REJECT") << " (expect PASS)\n\n";
    }
    // ---------- Тест 2: относительный порог ----------
    {
        patterns::TickVolumeConfig cfg;
        cfg.avg_ratio  = 0.8;
        cfg.avg_period = 10;
        patterns::TickVolumeFilter f(cfg);
        std::vector<core::Bar> bars;
        for (int i = 0; i < 9; ++i) bars.push_back(mk(i, 1000));
        std::cout << "--- Relative filter (ratio=0.8, period=10) ---\n";
        std::cout << "  baseline: 9 bars x vol=1000, avg=1000, threshold=800\n";
        auto b_low  = mk(100, 500);
        auto b_mid  = mk(101, 800);
        auto b_high = mk(102, 1500);
        auto check = [&](const core::Bar& b) {
            auto v = bars;
            v.push_back(b);
            return f.passAverage(v);
        };
        std::cout << "  +vol=500  : " << (check(b_low)  ? "PASS" : "REJECT") << " (expect REJECT)\n";
        std::cout << "  +vol=800  : " << (check(b_mid)  ? "PASS" : "REJECT") << " (expect PASS)\n";
        std::cout << "  +vol=1500 : " << (check(b_high) ? "PASS" : "REJECT") << " (expect PASS)\n\n";
    }
    // ---------- Тест 3: комбинация ----------
    {
        patterns::TickVolumeConfig cfg;
        cfg.min_volume = 300;
        cfg.avg_ratio  = 0.5;
        cfg.avg_period = 10;
        patterns::TickVolumeFilter f(cfg);
        std::vector<core::Bar> bars;
        for (int i = 0; i < 10; ++i) bars.push_back(mk(i, 1000));
        std::cout << "--- Combined filter (min=300, ratio=0.5) ---\n";
        auto b1 = mk(11, 200);
        auto b2 = mk(12, 400);
        auto b3 = mk(13, 600);
        {
            auto v = bars; v.push_back(b1);
            std::cout << "  vol=200  : " << (f.passAll(b1, v) ? "PASS" : "REJECT") << " (expect REJECT)\n";
        }
        {
            auto v = bars; v.push_back(b2);
            std::cout << "  vol=400  : " << (f.passAll(b2, v) ? "PASS" : "REJECT") << " (expect REJECT)\n";
        }
        {
            auto v = bars; v.push_back(b3);
            std::cout << "  vol=600  : " << (f.passAll(b3, v) ? "PASS" : "REJECT") << " (expect PASS)\n";
        }
        std::cout << "\n";
    }
    // ---------- Тест 4: защита от недостатка данных ----------
    {
        patterns::TickVolumeConfig cfg;
        cfg.avg_ratio  = 0.9;
        cfg.avg_period = 20;
        patterns::TickVolumeFilter f(cfg);
        std::vector<core::Bar> bars;
        for (int i = 0; i < 5; ++i) bars.push_back(mk(i, 100));
        std::cout << "--- Not enough data ---\n";
        std::cout << "  5 bars, period=20 : "
                  << (f.passAverage(bars) ? "PASS (fallback OK)" : "REJECT (FAIL)")
                  << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}