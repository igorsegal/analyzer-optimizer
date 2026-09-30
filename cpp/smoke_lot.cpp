// Smoke: BrokerLotRounder.
#include "validation/BrokerLotRounder.h"
#include <iostream>
#include <iomanip>
#include <cmath>
using namespace spartak;
static bool approx(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) < eps;
}
int main() {
    std::cout << "=== BrokerLotRounder smoke test ===\n\n";
    std::cout << std::fixed << std::setprecision(2);
    validation::LotRounderConfig cfg;
    cfg.min_lot  = 0.01;
    cfg.max_lot  = 50.0;
    cfg.lot_step = 0.01;
    validation::BrokerLotRounder r(cfg);
    // --- T1: round_down базовый ---
    {
        const double v = r.round_down(0.47619);
        std::cout << "T1 round_down(0.47619) : " << v
                  << "  (expect 0.47)  "
                  << (approx(v, 0.47) ? "OK" : "FAIL") << "\n";
    }
    // --- T2: round_down точно на шаге ---
    {
        const double v = r.round_down(0.50);
        std::cout << "T2 round_down(0.50)    : " << v
                  << "  (expect 0.50)  "
                  << (approx(v, 0.50) ? "OK" : "FAIL") << "\n";
    }
    // --- T3: round_down ниже min_lot -> 0 ---
    {
        const double v = r.round_down(0.005);
        std::cout << "T3 round_down(0.005)   : " << v
                  << "  (expect 0.00)  "
                  << (approx(v, 0.0) ? "OK" : "FAIL") << "\n";
    }
    // --- T4: round_down обрезка по max_lot ---
    {
        const double v = r.round_down(999.99);
        std::cout << "T4 round_down(999.99)  : " << v
                  << "  (expect 50.00)  "
                  << (approx(v, 50.0) ? "OK" : "FAIL") << "\n";
    }
    // --- T5: round_up базовый ---
    {
        const double v = r.round_up(0.471);
        std::cout << "T5 round_up(0.471)     : " << v
                  << "  (expect 0.48)  "
                  << (approx(v, 0.48) ? "OK" : "FAIL") << "\n";
    }
    // --- T6: round_up точно на шаге ---
    {
        const double v = r.round_up(0.50);
        std::cout << "T6 round_up(0.50)      : " << v
                  << "  (expect 0.50)  "
                  << (approx(v, 0.50) ? "OK" : "FAIL") << "\n";
    }
    // --- T7: is_valid_lot ---
    {
        const bool a = r.is_valid_lot(0.50);
        const bool b = r.is_valid_lot(0.471);
        const bool c = r.is_valid_lot(100.0);
        const bool d = r.is_valid_lot(0.0);
        std::cout << "T7 is_valid_lot(0.50)  : " << (a ? "YES" : "NO") << "  (expect YES)  " << (a ? "OK" : "FAIL") << "\n";
        std::cout << "   is_valid_lot(0.471) : " << (b ? "YES" : "NO") << "  (expect NO)   " << (!b ? "OK" : "FAIL") << "\n";
        std::cout << "   is_valid_lot(100.0) : " << (c ? "YES" : "NO") << "  (expect NO)   " << (!c ? "OK" : "FAIL") << "\n";
        std::cout << "   is_valid_lot(0.0)   : " << (d ? "YES" : "NO") << "  (expect NO)   " << (!d ? "OK" : "FAIL") << "\n";
    }
    // --- T8: steps_in ---
    {
        const auto s1 = r.steps_in(0.47);
        const auto s2 = r.steps_in(1.00);
        const auto s3 = r.steps_in(0.009);
        std::cout << "T8 steps_in(0.47)      : " << s1 << "  (expect 47)  " << (s1 == 47 ? "OK" : "FAIL") << "\n";
        std::cout << "   steps_in(1.00)      : " << s2 << "  (expect 100) " << (s2 == 100 ? "OK" : "FAIL") << "\n";
        std::cout << "   steps_in(0.009)     : " << s3 << "  (expect 0)   " << (s3 == 0 ? "OK" : "FAIL") << "\n";
    }
    // --- T9: шаг 0.1 (не 0.01) ---
    {
        validation::LotRounderConfig cfg2;
        cfg2.min_lot  = 0.1;
        cfg2.max_lot  = 10.0;
        cfg2.lot_step = 0.1;
        validation::BrokerLotRounder r2(cfg2);
        const double v = r2.round_down(0.47);
        std::cout << "T9 step=0.1 round(0.47): " << v
                  << "  (expect 0.40)  "
                  << (approx(v, 0.40) ? "OK" : "FAIL") << "\n";
    }
    // --- T10: floating-point чистота ---
    {
        // Проблема: 0.3 в double = 0.30000000000000004
        // После нормализации должно быть ровно 0.30
        validation::LotRounderConfig cfg3;
        cfg3.min_lot  = 0.01;
        cfg3.max_lot  = 50.0;
        cfg3.lot_step = 0.01;
        validation::BrokerLotRounder r3(cfg3);
        const double v = r3.round_down(0.301);
        std::cout << "T10 fp-clean (0.301)   : " << v
                  << "  (expect 0.30)  "
                  << (approx(v, 0.30) ? "OK" : "FAIL") << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}