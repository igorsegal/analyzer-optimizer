// Smoke: BreakEvenTransfer.
#include "position/BreakEvenTransfer.h"
#include <iostream>
#include <iomanip>
#include <cmath>
using namespace spartak;
static bool approx(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) < eps;
}
int main() {
    std::cout << "=== BreakEvenTransfer smoke test ===\n\n";
    std::cout << std::fixed << std::setprecision(5);
    position::BreakEvenConfig cfg;
    cfg.point = 0.00001;
    cfg.use_spread_in_be       = true;
    cfg.fallback_spread_points = 12;
    cfg.extra_buffer_points    = 0;
    cfg.clamp_buffer_points    = 1;
    position::BreakEvenTransfer be(cfg);
    {
        auto r = be.compute(core::OrderSide::Buy, 1.1000, 1.1030, 10, 1.0980);
        std::cout << "T1 BUY, no clamp      : new_sl=" << r.new_sl
                  << "  clamped=" << (r.clamped ? "Y" : "N")
                  << "  (expect 1.10010/N)  "
                  << (r.ok && !r.clamped && approx(r.new_sl, 1.10010) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = be.compute(core::OrderSide::Sell, 1.1000, 1.0970, 10, 1.1020);
        std::cout << "T2 SELL, no clamp     : new_sl=" << r.new_sl
                  << "  clamped=" << (r.clamped ? "Y" : "N")
                  << "  (expect 1.09990/N)  "
                  << (r.ok && !r.clamped && approx(r.new_sl, 1.09990) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = be.compute(core::OrderSide::Buy, 1.1000, 1.10005, 10, 1.0980);
        std::cout << "T3 BUY, clamped       : new_sl=" << r.new_sl
                  << "  clamped=" << (r.clamped ? "Y" : "N")
                  << "  (expect 1.10004/Y)  "
                  << (r.ok && r.clamped && approx(r.new_sl, 1.10004) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = be.compute(core::OrderSide::Sell, 1.1000, 1.09995, 10, 1.1020);
        std::cout << "T4 SELL, clamped      : new_sl=" << r.new_sl
                  << "  clamped=" << (r.clamped ? "Y" : "N")
                  << "  (expect 1.09996/Y)  "
                  << (r.ok && r.clamped && approx(r.new_sl, 1.09996) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = be.compute(core::OrderSide::Buy, 1.1000, 1.1030, 0, 1.0980);
        std::cout << "T5 BUY, fallback sprd : new_sl=" << r.new_sl
                  << "  (expect 1.10012)  "
                  << (r.ok && approx(r.new_sl, 1.10012) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        position::BreakEvenConfig cfg2;
        cfg2.point = 0.00001;
        cfg2.use_spread_in_be = false;
        position::BreakEvenTransfer be2(cfg2);
        auto r = be2.compute(core::OrderSide::Buy, 1.1000, 1.1030, 10, 1.0980);
        std::cout << "T6 no spread in BE    : new_sl=" << r.new_sl
                  << "  (expect 1.10000)  "
                  << (r.ok && approx(r.new_sl, 1.10000) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        position::BreakEvenConfig cfg3;
        cfg3.point = 0.00001;
        cfg3.use_spread_in_be = true;
        cfg3.extra_buffer_points = 5;
        position::BreakEvenTransfer be3(cfg3);
        auto r = be3.compute(core::OrderSide::Buy, 1.1000, 1.1030, 10, 1.0980);
        std::cout << "T7 extra_buffer=5pts  : new_sl=" << r.new_sl
                  << "  (expect 1.10015)  "
                  << (r.ok && approx(r.new_sl, 1.10015) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = be.compute(core::OrderSide::Buy, 1.1000, 1.1030, 10, 1.0980);
        std::cout << "T8 move_pts           : " << r.move_pts
                  << "  (expect 210)  "
                  << (approx(r.move_pts, 210.0) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = be.compute(core::OrderSide::Sell, 1.2000, 1.1970, 0, 1.2020);
        std::cout << "T9 SELL fallback      : new_sl=" << r.new_sl
                  << "  (expect 1.19988)  "
                  << (r.ok && approx(r.new_sl, 1.19988) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = be.compute(core::OrderSide::Buy, 0.0, 1.1000, 10, 1.0980);
        std::cout << "T10 zero entry        : ok=" << (r.ok ? "Y" : "N")
                  << "  (expect N)  "
                  << (!r.ok ? "OK" : "FAIL")
                  << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}