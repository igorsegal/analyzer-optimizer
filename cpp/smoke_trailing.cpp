// Smoke: TrailingStopManager.
#include "position/TrailingStopManager.h"
#include <iostream>
#include <iomanip>
#include <cmath>
using namespace spartak;
static bool approx(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) < eps;
}
static core::Bar mk(double h, double l) {
    return core::Bar{0, 1.0, h, l, 1.0, 0, 0};
}
int main() {
    std::cout << "=== TrailingStopManager smoke test ===\n\n";
    std::cout << std::fixed << std::setprecision(5);
    position::TrailingConfig cfg;
    cfg.point = 0.00001;
    cfg.trailing_distance_points = 30;
    cfg.activation_points        = 0;
    cfg.min_step_points          = 1;
    position::TrailingStopManager t(cfg);
    {
        auto r = t.compute(core::OrderSide::Buy, 1.0980, 1.1000, mk(1.1050, 1.0980));
        std::cout << "T1 BUY trail up        : moved=" << (r.moved ? "Y" : "N")
                  << "  new_sl=" << r.new_sl
                  << "  (expect Y/1.10470)  "
                  << (r.moved && approx(r.new_sl, 1.10470) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = t.compute(core::OrderSide::Buy, 1.10480, 1.1000, mk(1.1050, 1.0980));
        std::cout << "T2 BUY no move         : moved=" << (r.moved ? "Y" : "N")
                  << "  (expect N)  "
                  << (!r.moved ? "OK" : "FAIL")
                  << "\n";
    }
    {
        // candidate=1.09840, current=1.09830, diff=10 pts > min_step=1 pt -> Y
        auto r = t.compute(core::OrderSide::Buy, 1.09830, 1.1000, mk(1.09870, 1.0980));
        std::cout << "T3 BUY 10pts move      : moved=" << (r.moved ? "Y" : "N")
                  << "  new_sl=" << r.new_sl
                  << "  (expect Y/1.09840)  "
                  << (r.moved && approx(r.new_sl, 1.09840) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = t.compute(core::OrderSide::Buy, 1.09810, 1.1000, mk(1.09890, 1.0980));
        std::cout << "T4 BUY 5pts move       : moved=" << (r.moved ? "Y" : "N")
                  << "  new_sl=" << r.new_sl
                  << "  (expect Y/1.09860)  "
                  << (r.moved && approx(r.new_sl, 1.09860) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = t.compute(core::OrderSide::Sell, 1.1020, 1.1000, mk(1.1020, 1.0950));
        std::cout << "T5 SELL trail down     : moved=" << (r.moved ? "Y" : "N")
                  << "  new_sl=" << r.new_sl
                  << "  (expect Y/1.09530)  "
                  << (r.moved && approx(r.new_sl, 1.09530) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = t.compute(core::OrderSide::Sell, 1.09520, 1.1000, mk(1.1020, 1.0950));
        std::cout << "T6 SELL no move        : moved=" << (r.moved ? "Y" : "N")
                  << "  (expect N)  "
                  << (!r.moved ? "OK" : "FAIL")
                  << "\n";
    }
    {
        position::TrailingConfig cfg2 = cfg;
        cfg2.activation_points = 50;
        position::TrailingStopManager t2(cfg2);
        // activation=50 pts -> threshold=1.10050; bar.high=1.10040 < 1.10050 -> не активировано
        auto r = t2.compute(core::OrderSide::Buy, 1.0980, 1.1000, mk(1.10040, 1.0980));
        std::cout << "T7 BUY activation off  : moved=" << (r.moved ? "Y" : "N")
                  << "  (expect N)  "
                  << (!r.moved ? "OK" : "FAIL")
                  << "\n";
    }
    {
        position::TrailingConfig cfg2 = cfg;
        cfg2.activation_points = 50;
        position::TrailingStopManager t2(cfg2);
        auto r = t2.compute(core::OrderSide::Buy, 1.0980, 1.1000, mk(1.1055, 1.0980));
        std::cout << "T8 BUY activation on   : moved=" << (r.moved ? "Y" : "N")
                  << "  new_sl=" << r.new_sl
                  << "  (expect Y/1.10520)  "
                  << (r.moved && approx(r.new_sl, 1.10520) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        auto r = t.compute(core::OrderSide::Buy, 1.0980, 1.1000, mk(1.1050, 1.0980));
        std::cout << "T9 move_pts diag       : move_pts=" << r.move_pts
                  << "  (expect 670)  "
                  << (approx(r.move_pts, 670.0) ? "OK" : "FAIL")
                  << "\n";
    }
    {
        position::TrailingConfig cfg2 = cfg;
        cfg2.activation_points = 50;
        position::TrailingStopManager t2(cfg2);
        auto r = t2.compute(core::OrderSide::Sell, 1.1020, 1.1000, mk(1.1020, 1.0945));
        std::cout << "T10 SELL activation    : moved=" << (r.moved ? "Y" : "N")
                  << "  new_sl=" << r.new_sl
                  << "  (expect Y/1.09480)  "
                  << (r.moved && approx(r.new_sl, 1.09480) ? "OK" : "FAIL")
                  << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}