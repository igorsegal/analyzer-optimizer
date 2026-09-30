// Smoke: MoneyRiskCalculator.
#include "validation/MoneyRiskCalculator.h"
#include <iostream>
#include <iomanip>
#include <cmath>
using namespace spartak;
static bool approx(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) < eps;
}
int main() {
    std::cout << "=== MoneyRiskCalculator smoke test ===\n\n";
    std::cout << std::fixed << std::setprecision(5);
    validation::MoneyRiskConfig cfg;
    cfg.risk_percent  = 1.0;
    cfg.contract_size = 100'000.0;
    cfg.point         = 0.00001;
    validation::MoneyRiskCalculator calc(cfg);
    // --- T1: базовый расчёт ---
    // balance=10000, risk=1% -> 100 USD
    // entry=1.1000, stop=1.0980, dist=0.0020
    // raw_lot = 100 / (0.0020 * 100000) = 0.5
    {
        const double lot = calc.calcRawLot(10000.0, 1.1000, 1.0980);
        std::cout << "T1 raw lot (10000, 1.10, 1.098): " << lot
                  << "  (expect 0.50)  "
                  << (approx(lot, 0.50) ? "OK" : "FAIL") << "\n";
    }
    // --- T2: другой риск ---
    // balance=50000, risk=1% -> 500 USD
    // entry=1.2000, stop=1.1950, dist=0.0050
    // raw_lot = 500 / (0.0050 * 100000) = 1.0
    {
        const double lot = calc.calcRawLot(50000.0, 1.2000, 1.1950);
        std::cout << "T2 raw lot (50000, 1.20, 1.195): " << lot
                  << "  (expect 1.00)  "
                  << (approx(lot, 1.00) ? "OK" : "FAIL") << "\n";
    }
    // --- T3: stop distance = 0 ---
    {
        const double lot = calc.calcRawLot(10000.0, 1.1000, 1.1000);
        std::cout << "T3 zero distance               : " << lot
                  << "  (expect 0.00)  "
                  << (approx(lot, 0.0) ? "OK" : "FAIL") << "\n";
    }
    // --- T4: stopDistance helper ---
    {
        const double d = validation::MoneyRiskCalculator::stopDistance(1.1000, 1.0980);
        std::cout << "T4 stopDistance (1.10-1.098)   : " << d
                  << "  (expect 0.00200)  "
                  << (approx(d, 0.0020) ? "OK" : "FAIL") << "\n";
    }
    // --- T5: commission учитывается ---
    // Идеальный случай: lot=0.5, commission_per_lot=10 -> com=5 USD
    // risk_net = 100 - 5 = 95
    // lot = 95 / (0.0020 * 100000) = 0.475
    {
        const double lot = calc.calcRawLotWithCommission(10000.0, 1.1000, 1.0980, 10.0);
        const double expected_t5 = 100.0 / 210.0;   // 0.476190...
        std::cout << "T5 with commission (10/lot)    : " << lot
                  << "  (expect " << expected_t5 << ")  "
                  << (approx(lot, expected_t5, 1e-5) ? "OK" : "FAIL") << "\n";
    }
    // --- T6: большая комиссия съедает весь риск ---
    {
        const double lot = calc.calcRawLotWithCommission(10.0, 1.1000, 1.0980, 1000.0);
        std::cout << "T6 huge commission             : " << lot
                  << "  (expect 0.00)  "
                  << (approx(lot, 0.0) ? "OK" : "FAIL") << "\n";
    }
    // --- T7: calcRiskAmount обратной формулой ---
    // lot=0.5, dist=0.0020 -> risk = 0.0020 * 0.5 * 100000 = 100 USD
    {
        const double risk = calc.calcRiskAmount(0.5, 1.1000, 1.0980);
        std::cout << "T7 riskAmount(0.5 lots)        : " << risk
                  << "  (expect 100)  "
                  << (approx(risk, 100.0) ? "OK" : "FAIL") << "\n";
    }
    // --- T8: риск меньше 100, если лот меньше ---
    {
        const double risk = calc.calcRiskAmount(0.25, 1.1000, 1.0980);
        std::cout << "T8 riskAmount(0.25 lots)       : " << risk
                  << "  (expect 50)  "
                  << (approx(risk, 50.0) ? "OK" : "FAIL") << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}