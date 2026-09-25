// Smoke: SessionTimeFilter.
#include "validation/SessionTimeFilter.h"
#include <iostream>
#include <cstdint>
using namespace spartak;
// Пример: 2024-01-01 12:00 UTC = 1704110400000 ms
// Каждый час = 3600000 ms
static int64_t hours_after_midnight(int h) {
    return 1704067200000LL + static_cast<int64_t>(h) * 3600000LL;   // 2024-01-01 00:00 UTC
}
int main() {
    std::cout << "=== SessionTimeFilter smoke test ===\n\n";
    // --- T1: часы 9..17 (London+NY) ---
    {
        validation::SessionConfig cfg;
        cfg.begin_hour_utc = 9;
        cfg.end_hour_utc   = 17;
        validation::SessionTimeFilter f(cfg);
        std::cout << "T1 session [9, 17) ---\n";
        std::cout << "  hour=8  : " << (f.pass(hours_after_midnight(8))  ? "PASS" : "REJECT") << " (expect REJECT)\n";
        std::cout << "  hour=9  : " << (f.pass(hours_after_midnight(9))  ? "PASS" : "REJECT") << " (expect PASS)\n";
        std::cout << "  hour=16 : " << (f.pass(hours_after_midnight(16)) ? "PASS" : "REJECT") << " (expect PASS)\n";
        std::cout << "  hour=17 : " << (f.pass(hours_after_midnight(17)) ? "PASS" : "REJECT") << " (expect REJECT)\n";
        std::cout << "  hour=23 : " << (f.pass(hours_after_midnight(23)) ? "PASS" : "REJECT") << " (expect REJECT)\n\n";
    }
    // --- T2: сессия через полночь [22, 6) ---
    {
        validation::SessionConfig cfg;
        cfg.begin_hour_utc = 22;
        cfg.end_hour_utc   = 6;
        validation::SessionTimeFilter f(cfg);
        std::cout << "T2 session [22, 6) ---\n";
        std::cout << "  hour=21 : " << (f.pass(hours_after_midnight(21)) ? "PASS" : "REJECT") << " (expect REJECT)\n";
        std::cout << "  hour=22 : " << (f.pass(hours_after_midnight(22)) ? "PASS" : "REJECT") << " (expect PASS)\n";
        std::cout << "  hour=23 : " << (f.pass(hours_after_midnight(23)) ? "PASS" : "REJECT") << " (expect PASS)\n";
        std::cout << "  hour=0  : " << (f.pass(hours_after_midnight(0))  ? "PASS" : "REJECT") << " (expect PASS)\n";
        std::cout << "  hour=5  : " << (f.pass(hours_after_midnight(5))  ? "PASS" : "REJECT") << " (expect PASS)\n";
        std::cout << "  hour=6  : " << (f.pass(hours_after_midnight(6))  ? "PASS" : "REJECT") << " (expect REJECT)\n\n";
    }
    // --- T3: фильтр выключен ---
    {
        validation::SessionConfig cfg;
        cfg.enabled = false;
        validation::SessionTimeFilter f(cfg);
        std::cout << "T3 disabled filter ---\n";
        std::cout << "  hour=3  : " << (f.pass(hours_after_midnight(3))  ? "PASS" : "REJECT") << " (expect PASS)\n\n";
    }
    // --- T4: пустое окно begin==end ---
    {
        validation::SessionConfig cfg;
        cfg.begin_hour_utc = 12;
        cfg.end_hour_utc   = 12;
        validation::SessionTimeFilter f(cfg);
        std::cout << "T4 begin==end (24h) ---\n";
        std::cout << "  hour=3  : " << (f.pass(hours_after_midnight(3))  ? "PASS" : "REJECT") << " (expect PASS)\n";
        std::cout << "  hour=15 : " << (f.pass(hours_after_midnight(15)) ? "PASS" : "REJECT") << " (expect PASS)\n\n";
    }
    // --- T5: hour_utc() helper ---
    {
        std::cout << "T5 hour_utc() helper ---\n";
        for (int h : {0, 6, 12, 18, 23}) {
            const int got = validation::SessionTimeFilter::hour_utc(hours_after_midnight(h));
            std::cout << "  hours_after_midnight(" << h << ") -> " << got
                      << "  " << (got == h ? "OK" : "FAIL") << "\n";
        }
    }
    std::cout << "\nDone.\n";
    return 0;
}