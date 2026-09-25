// =============================================================================
//  SPARTAK :: validation/MoneyRiskCalculator.h
//  Расчёт объёма позиции из процента риска.
//
//  Формула:
//    risk_money = balance * risk_percent / 100
//    stop_dist  = |entry - stop|
//    raw_lot    = risk_money / (stop_dist * contract_size)
//
//  Возвращает СЫРОЙ лот (без нормализации). Нормализацию до шага делает
//  отдельный модуль BrokerLotRounder — так проще тестировать каждый шаг.
//
//  Есть вариант с учётом комиссии: она вычитается из risk_money до деления.
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstdint>
namespace spartak::validation {
// -----------------------------------------------------------------------------
// Конфиг.
// -----------------------------------------------------------------------------
struct MoneyRiskConfig {
    double risk_percent  = 1.0;        // % от баланса на сделку
    double contract_size = 100'000.0;  // размер контракта (1 лот)
    double point         = 0.00001;
};
// -----------------------------------------------------------------------------
// MoneyRiskCalculator — stateless.
// -----------------------------------------------------------------------------
class MoneyRiskCalculator {
public:
    explicit MoneyRiskCalculator(MoneyRiskConfig cfg = {});
    // Сырой лот без учёта комиссии.
    [[nodiscard]] double calcRawLot(double balance,
                                    double entry,
                                    double stop) const noexcept;
    // Сырой лот с учётом круговой комиссии.
    // Итеративно: комиссия зависит от лота, а лот от комиссии.
    [[nodiscard]] double calcRawLotWithCommission(double balance,
                                                  double entry,
                                                  double stop,
                                                  double commission_per_lot) const noexcept;
    // Риск по факту: сколько денег потеряем на этом лоте при стопе.
    [[nodiscard]] double calcRiskAmount(double lots,
                                        double entry,
                                        double stop) const noexcept;
    // Дистанция до стопа в цене.
    [[nodiscard]] static double stopDistance(double entry, double stop) noexcept;
    const MoneyRiskConfig& config() const noexcept { return cfg_; }
private:
    MoneyRiskConfig cfg_;
};
} // namespace spartak::validation