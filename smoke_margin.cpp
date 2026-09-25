// Smoke: MarginCallChecker.
#include "validation/MarginCallChecker.h"
#include <iostream>
#include <iomanip>
#include <cmath>
using namespace spartak;
using validation::MarginCheckResult;
static bool approx(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) < eps;
}
static const char* name(MarginCheckResult r) {
    return validation::to_string(r);
}
int main() {
    std::cout << "=== MarginCallChecker smoke test ===\n\n";
    std::cout << std::fixed << std::setprecision(2);
    validation::MarginConfig cfg;
    cfg.contract_size = 100'000.0;
    validation::MarginCallChecker chk(cfg);
    // --- T1: calcMargin 1 лот, плечо 500 ---
    {
        const double m = chk.calcMargin(1.0, 500.0);
        std::cout << "T1 calcMargin(1.0, 500)      : " << m
                  << "  (expect 200.00)  "
                  << (approx(m, 200.0) ? "OK" : "FAIL") << "\n";
    }
    // --- T2: calcMargin 0.5 лота ---
    {
        const double m = chk.calcMargin(0.5, 500.0);
        std::cout << "T2 calcMargin(0.5, 500)      : " << m
                  << "  (expect 100.00)  "
                  << (approx(m, 100.0) ? "OK" : "FAIL") << "\n";
    }
    // --- T3: calcMargin 1 лот, плечо 100 ---
    {
        const double m = chk.calcMargin(1.0, 100.0);
        std::cout << "T3 calcMargin(1.0, 100)      : " << m
                  << "  (expect 1000.00)  "
                  << (approx(m, 1000.0) ? "OK" : "FAIL") << "\n";
    }
    // --- T4: marginLevelPct ---
    {
        const double lvl = chk.calcMarginLevelPct(10000.0, 200.0);
        std::cout << "T4 marginLevel(10000/200)    : " << lvl
                  << "  (expect 5000.00)  "
                  << (approx(lvl, 5000.0, 1.0) ? "OK" : "FAIL") << "\n";
    }
    // --- T5: marginLevelPct при большой марже ---
    {
        const double lvl = chk.calcMarginLevelPct(10000.0, 2000.0);
        std::cout << "T5 marginLevel(10000/2000)   : " << lvl
                  << "  (expect 500.00)  "
                  << (approx(lvl, 500.0) ? "OK" : "FAIL") << "\n";
    }
    // --- T6: check = Ok ---
    {
        validation::MarginAccountState acc;
        acc.equity = 10000.0;
        acc.free_margin = 10000.0;
        acc.margin_used = 0.0;
        acc.leverage = 500.0;
        acc.min_margin_level_pct = 5000.0;
        const auto r = chk.check(1.0, acc);
        std::cout << "T6 check(1.0) -> " << name(r) << "  (expect Ok)  "
                  << (r == MarginCheckResult::Ok ? "OK" : "FAIL") << "\n";
    }
    // --- T7: NotEnoughFreeMargin ---
    {
        validation::MarginAccountState acc;
        acc.equity = 10000.0;
        acc.free_margin = 50.0;
        acc.margin_used = 9950.0;
        acc.leverage = 500.0;
        acc.min_margin_level_pct = 5000.0;
        const auto r = chk.check(1.0, acc);   // new_margin=200 > free=50
        std::cout << "T7 check(1.0, free=50) -> " << name(r)
                  << "  (expect NotEnoughFreeMargin)  "
                  << (r == MarginCheckResult::NotEnoughFreeMargin ? "OK" : "FAIL") << "\n";
    }
    // --- T8: MarginCall (level < min) ---
    {
        validation::MarginAccountState acc;
        acc.equity = 10000.0;
        acc.free_margin = 100.0;
        acc.margin_used = 9900.0;             // уже загружено
        acc.leverage = 500.0;
        acc.min_margin_level_pct = 5000.0;
        // volume=0.01 -> new_margin=2; total=9902; level=10000/9902*100=100.99% < 5000% -> MarginCall
        const auto r = chk.check(0.01, acc);
        std::cout << "T8 check(0.01, used=9900) -> " << name(r)
                  << "  (expect MarginCall)  "
                  << (r == MarginCheckResult::MarginCall ? "OK" : "FAIL") << "\n";
    }
    // --- T9: volume=0 -> Ok ---
    {
        validation::MarginAccountState acc;
        acc.free_margin = 0.0;
        const auto r = chk.check(0.0, acc);
        std::cout << "T9 check(0.0) -> " << name(r) << "  (expect Ok)  "
                  << (r == MarginCheckResult::Ok ? "OK" : "FAIL") << "\n";
    }
    // --- T10: большой лот с малым балансом -> MarginCall ---
    {
        validation::MarginAccountState acc;
        acc.equity = 10000.0;
        acc.free_margin = 1'000'000.0;        // денег хватает
        acc.margin_used = 0.0;
        acc.leverage = 500.0;
        acc.min_margin_level_pct = 5000.0;
        // volume=10 -> new_margin=2000; total=2000; level=10000/2000*100=500% < 5000% -> MarginCall
        const auto r = chk.check(10.0, acc);
        std::cout << "T10 check(10.0) -> " << name(r)
                  << "  (expect MarginCall)  "
                  << (r == MarginCheckResult::MarginCall ? "OK" : "FAIL") << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}