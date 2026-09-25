// Smoke: CloseCommissionCalculator.
#include "position/CloseCommissionCalculator.h"
#include <iostream>
#include <iomanip>
#include <cmath>
using namespace spartak;
static bool approx(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) < eps;
}
int main() {
    std::cout << "=== CloseCommissionCalculator smoke test ===\n\n";
    std::cout << std::fixed << std::setprecision(2);
    position::CommissionConfig cfg;
    cfg.commission_per_lot = 5.0;
    position::CloseCommissionCalculator c(cfg);
    // --- T1: closeCommission 1.0 лот = 2.50 ---
    {
        const double v = c.closeCommission(1.0);
        std::cout << "T1 close(1.0)          : " << v
                  << "  (expect 2.50)  "
                  << (approx(v, 2.5) ? "OK" : "FAIL") << "\n";
    }
    // --- T2: closeCommission 0.5 лота = 1.25 ---
    {
        const double v = c.closeCommission(0.5);
        std::cout << "T2 close(0.5)          : " << v
                  << "  (expect 1.25)  "
                  << (approx(v, 1.25) ? "OK" : "FAIL") << "\n";
    }
    // --- T3: openCommission 1.0 лот = 2.50 ---
    {
        const double v = c.openCommission(1.0);
        std::cout << "T3 open(1.0)           : " << v
                  << "  (expect 2.50)  "
                  << (approx(v, 2.5) ? "OK" : "FAIL") << "\n";
    }
    // --- T4: roundCommission 1.0 лот = 5.00 ---
    {
        const double v = c.roundCommission(1.0);
        std::cout << "T4 round(1.0)          : " << v
                  << "  (expect 5.00)  "
                  << (approx(v, 5.0) ? "OK" : "FAIL") << "\n";
    }
    // --- T5: open + close = round ---
    {
        const double o = c.openCommission(2.5);
        const double cl = c.closeCommission(2.5);
        const double rt = c.roundCommission(2.5);
        std::cout << "T5 open+close=round    : o=" << o << " cl=" << cl << " rt=" << rt
                  << "  " << (approx(o + cl, rt) ? "OK" : "FAIL") << "\n";
    }
    // --- T6: close(0.0) = 0 ---
    {
        const double v = c.closeCommission(0.0);
        std::cout << "T6 close(0.0)          : " << v
                  << "  (expect 0.00)  "
                  << (approx(v, 0.0) ? "OK" : "FAIL") << "\n";
    }
    // --- T7: close(-1.0) = 0 (защита) ---
    {
        const double v = c.closeCommission(-1.0);
        std::cout << "T7 close(-1.0)         : " << v
                  << "  (expect 0.00)  "
                  << (approx(v, 0.0) ? "OK" : "FAIL") << "\n";
    }
    // --- T8: разные commission_per_lot ---
    {
        position::CommissionConfig cfg2;
        cfg2.commission_per_lot = 10.0;
        position::CloseCommissionCalculator c2(cfg2);
        const double v = c2.closeCommission(0.5);
        std::cout << "T8 comm=10 close(0.5)  : " << v
                  << "  (expect 2.50)  "
                  << (approx(v, 2.5) ? "OK" : "FAIL") << "\n";
    }
    // --- T9: close(2.0) при comm=5 = 5.00 ---
    {
        const double v = c.closeCommission(2.0);
        std::cout << "T9 close(2.0)          : " << v
                  << "  (expect 5.00)  "
                  << (approx(v, 5.0) ? "OK" : "FAIL") << "\n";
    }
    // --- T10: суммарная комиссия за partial + full close ---
    // partial 0.5 + full 0.5 = close(0.5) + close(0.5) = 1.25 + 1.25 = 2.50
    {
        const double v = c.closeCommission(0.5) + c.closeCommission(0.5);
        std::cout << "T10 partial+full close : " << v
                  << "  (expect 2.50)  "
                  << (approx(v, 2.5) ? "OK" : "FAIL") << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}