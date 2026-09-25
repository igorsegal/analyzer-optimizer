// Smoke: SwapAccrualTracker.
#include "position/SwapAccrualTracker.h"
#include <iostream>
#include <iomanip>
#include <cmath>
using namespace spartak;
static bool approx(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) < eps;
}
static constexpr int64_t T0 = 1704067200000LL;
int main() {
    std::cout << "=== SwapAccrualTracker smoke test ===\n\n";
    std::cout << std::fixed << std::setprecision(2);
    position::SwapConfig cfg;
    cfg.swap_long_points  = -7.0;
    cfg.swap_short_points = -2.0;
    cfg.point             = 0.00001;
    cfg.contract_size     = 100'000.0;
    position::SwapAccrualTracker s(cfg);
    {
        const auto d = position::SwapAccrualTracker::day_of(T0);
        std::cout << "T1 day_of(T0)         : " << d
                  << "  (expect 19723)  "
                  << (d == 19723 ? "OK" : "FAIL") << "\n";
    }
    {
        auto r = s.compute(core::OrderSide::Buy, 1.0, -1, T0);
        std::cout << "T2 init (last=-1)     : accrued=" << (r.accrued ? "Y" : "N")
                  << "  nights=" << r.nights
                  << "  new_day=" << r.new_day
                  << "  (expect N/0/19723)  "
                  << (!r.accrued && r.nights == 0 && r.new_day == 19723 ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = s.compute(core::OrderSide::Buy, 1.0, 19723, T0 + 3600000);
        std::cout << "T3 same day           : accrued=" << (r.accrued ? "Y" : "N")
                  << "  nights=" << r.nights
                  << "  (expect N/0)  "
                  << (!r.accrued && r.nights == 0 ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = s.compute(core::OrderSide::Buy, 1.0, 19723, T0 + 86'400'000);
        std::cout << "T4 1 night BUY 1.0    : nights=" << r.nights
                  << "  money=" << r.money
                  << "  new_day=" << r.new_day
                  << "  (expect 1/-7.00/19724)  "
                  << (r.accrued && r.nights == 1 && approx(r.money, -7.0) && r.new_day == 19724 ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = s.compute(core::OrderSide::Sell, 0.5, 19723, T0 + 86'400'000);
        std::cout << "T5 1 night SELL 0.5   : nights=" << r.nights
                  << "  money=" << r.money
                  << "  (expect 1/-1.00)  "
                  << (r.accrued && r.nights == 1 && approx(r.money, -1.0) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = s.compute(core::OrderSide::Buy, 1.0, 19723, T0 + 3 * 86'400'000LL);
        std::cout << "T6 3 nights BUY 1.0   : nights=" << r.nights
                  << "  money=" << r.money
                  << "  (expect 3/-21.00)  "
                  << (r.accrued && r.nights == 3 && approx(r.money, -21.0) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = s.compute(core::OrderSide::Buy, 2.0, 19723, T0 + 3 * 86'400'000LL);
        std::cout << "T7 3 nights BUY 2.0   : money=" << r.money
                  << "  (expect -42.00)  "
                  << (approx(r.money, -42.0) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = s.compute(core::OrderSide::Buy, 0.0, 19723, T0 + 86'400'000);
        std::cout << "T8 zero volume        : nights=" << r.nights
                  << "  money=" << r.money
                  << "  (expect 1/0.00)  "
                  << (r.accrued && r.nights == 1 && approx(r.money, 0.0) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = s.compute(core::OrderSide::Buy, 1.0, 19723, T0 + 7 * 86'400'000LL);
        std::cout << "T9 7 nights BUY 1.0   : nights=" << r.nights
                  << "  money=" << r.money
                  << "  (expect 7/-49.00)  "
                  << (r.accrued && r.nights == 7 && approx(r.money, -49.0) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        position::SwapConfig cfg2;
        cfg2.swap_long_points  = +3.0;
        cfg2.swap_short_points = -1.0;
        cfg2.point             = 0.00001;
        cfg2.contract_size     = 100'000.0;
        position::SwapAccrualTracker s2(cfg2);
        auto r = s2.compute(core::OrderSide::Buy, 1.0, 19723, T0 + 86'400'000);
        std::cout << "T10 positive swap     : money=" << r.money
                  << "  (expect +3.00)  "
                  << (approx(r.money, 3.0) ? "OK" : "FAIL")
                  << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}