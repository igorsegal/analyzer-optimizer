// Smoke: SlippageToleranceChecker.
#include "validation/SlippageToleranceChecker.h"
#include <iostream>
#include <iomanip>
using namespace spartak;
int main() {
    std::cout << "=== SlippageToleranceChecker smoke test ===\n\n";
    std::cout << std::fixed << std::setprecision(5);
    validation::SlippageConfig cfg;
    cfg.max_points = 5;
    cfg.point      = 0.00001;
    validation::SlippageToleranceChecker s(cfg);
    {
        const int pts = s.slippagePoints(1.10000, 1.10000);
        const bool p  = s.pass(1.10000, 1.10000);
        std::cout << "T1 diff 0 pts         : " << pts
                  << "  pass=" << (p ? "Y" : "N")
                  << "  (expect 0/Y)  "
                  << (pts == 0 && p ? "OK" : "FAIL") << "\n";
    }
    {
        const int pts = s.slippagePoints(1.10000, 1.10003);
        const bool p  = s.pass(1.10000, 1.10003);
        std::cout << "T2 diff 3 pts         : " << pts
                  << "  pass=" << (p ? "Y" : "N")
                  << "  (expect 3/Y)  "
                  << (pts == 3 && p ? "OK" : "FAIL") << "\n";
    }
    {
        const int pts = s.slippagePoints(1.10000, 1.10005);
        const bool p  = s.pass(1.10000, 1.10005);
        std::cout << "T3 diff 5 pts (edge)  : " << pts
                  << "  pass=" << (p ? "Y" : "N")
                  << "  (expect 5/Y)  "
                  << (pts == 5 && p ? "OK" : "FAIL") << "\n";
    }
    {
        const int pts = s.slippagePoints(1.10000, 1.10006);
        const bool p  = s.pass(1.10000, 1.10006);
        std::cout << "T4 diff 6 pts         : " << pts
                  << "  pass=" << (p ? "Y" : "N")
                  << "  (expect 6/N)  "
                  << (pts == 6 && !p ? "OK" : "FAIL") << "\n";
    }
    {
        const int pts = s.slippagePoints(1.10000, 1.09997);
        const bool p  = s.pass(1.10000, 1.09997);
        std::cout << "T5 diff -3 pts        : " << pts
                  << "  pass=" << (p ? "Y" : "N")
                  << "  (expect 3/Y)  "
                  << (pts == 3 && p ? "OK" : "FAIL") << "\n";
    }
    {
        const int pts = s.slippagePoints(1.10000, 1.10050);
        const bool p  = s.pass(1.10000, 1.10050);
        std::cout << "T6 diff 50 pts        : " << pts
                  << "  pass=" << (p ? "Y" : "N")
                  << "  (expect 50/N)  "
                  << (pts == 50 && !p ? "OK" : "FAIL") << "\n";
    }
    {
        validation::SlippageConfig cfg2;
        cfg2.max_points = 5;
        cfg2.point      = 0.01;
        validation::SlippageToleranceChecker s2(cfg2);
        const int pts = s2.slippagePoints(1.00, 1.06);
        std::cout << "T7 point=0.01, diff=6 : " << pts
                  << "  (expect 6)  "
                  << (pts == 6 ? "OK" : "FAIL") << "\n";
    }
    {
        validation::SlippageConfig cfg3;
        cfg3.max_points = 0;
        cfg3.point      = 0.00001;
        validation::SlippageToleranceChecker s3(cfg3);
        const bool p_ok = s3.pass(1.10000, 1.10000);
        const bool p_no = s3.pass(1.10000, 1.10001);
        std::cout << "T8 max=0, no diff     : " << (p_ok ? "PASS" : "REJECT") << " (expect PASS)  " << (p_ok ? "OK" : "FAIL") << "\n";
        std::cout << "   max=0, 1pt diff    : " << (p_no ? "PASS" : "REJECT") << " (expect REJECT)  " << (!p_no ? "OK" : "FAIL") << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}