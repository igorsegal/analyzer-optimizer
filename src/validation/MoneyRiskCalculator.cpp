#include "validation/MoneyRiskCalculator.h"
#include <cmath>
#include <stdexcept>
namespace spartak::validation {
MoneyRiskCalculator::MoneyRiskCalculator(MoneyRiskConfig cfg)
    : cfg_(cfg) {
    if (cfg_.risk_percent <= 0.0 || cfg_.risk_percent > 100.0)
        throw std::invalid_argument("MoneyRiskConfig::risk_percent must be in (0,100]");
    if (cfg_.contract_size <= 0.0)
        throw std::invalid_argument("MoneyRiskConfig::contract_size must be > 0");
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("MoneyRiskConfig::point must be > 0");
}
double MoneyRiskCalculator::stopDistance(double entry, double stop) noexcept {
    return std::fabs(entry - stop);
}
// -----------------------------------------------------------------------------
// calcRawLot — базовая формула.
// -----------------------------------------------------------------------------
double MoneyRiskCalculator::calcRawLot(double balance,
                                       double entry,
                                       double stop) const noexcept {
    const double stop_dist = stopDistance(entry, stop);
    if (stop_dist <= 0.0) return 0.0;
    const double risk_money = balance * cfg_.risk_percent / 100.0;
    if (risk_money <= 0.0) return 0.0;
    return risk_money / (stop_dist * cfg_.contract_size);
}
// -----------------------------------------------------------------------------
// calcRawLotWithCommission — итеративная коррекция на комиссию.
//
// Три шага: начальная оценка без комиссии, потом два уточнения.
// -----------------------------------------------------------------------------
double MoneyRiskCalculator::calcRawLotWithCommission(double balance,
                                                     double entry,
                                                     double stop,
                                                     double commission_per_lot) const noexcept {
    const double stop_dist = stopDistance(entry, stop);
    if (stop_dist <= 0.0) return 0.0;
    const double risk_total = balance * cfg_.risk_percent / 100.0;
    if (risk_total <= 0.0) return 0.0;
    // Начальная оценка
    double lot = risk_total / (stop_dist * cfg_.contract_size);
    if (lot <= 0.0) return 0.0;
    // Итеративное уточнение с учётом комиссии
    for (int i = 0; i < 3; ++i) {
        const double commission = lot * commission_per_lot;
        const double risk_net = risk_total - commission;
        if (risk_net <= 0.0) return 0.0;
        lot = risk_net / (stop_dist * cfg_.contract_size);
    }
    return lot;
}
// -----------------------------------------------------------------------------
// calcRiskAmount — обратная формула: сколько потеряем при срабатывании SL.
// -----------------------------------------------------------------------------
double MoneyRiskCalculator::calcRiskAmount(double lots,
                                           double entry,
                                           double stop) const noexcept {
    const double stop_dist = stopDistance(entry, stop);
    return stop_dist * lots * cfg_.contract_size;
}
} // namespace spartak::validation