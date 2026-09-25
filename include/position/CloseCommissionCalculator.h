// =============================================================================
//  SPARTAK :: position/CloseCommissionCalculator.h
//  Расчёт комиссии при закрытии (частичном или полном).
//
//  По методике:
//    - комиссия круговая: половина списывается при входе, половина при выходе;
//    - при частичном закрытии комиссия пропорциональна закрываемому объёму;
//    - commission_per_lot задаётся брокером (обычно $3-10 за круг на лот).
//
//  Модуль считает ТОЛЬКО закрывающую часть. Открывающая была списана
//  в момент входа и хранится в ManagedPosition::commission_paid.
// =============================================================================
#pragma once
#include <cstdint>
namespace spartak::position {
// -----------------------------------------------------------------------------
// Конфиг.
// -----------------------------------------------------------------------------
struct CommissionConfig {
    double commission_per_lot = 5.0;    // круговая комиссия за 1.0 лот
    bool   round_only          = false; // true = считать только половину (закрытие)
};
// -----------------------------------------------------------------------------
// CloseCommissionCalculator — stateless.
// -----------------------------------------------------------------------------
class CloseCommissionCalculator {
public:
    explicit CloseCommissionCalculator(CommissionConfig cfg = {});
    // Комиссия за закрытие заданного объёма.
    //   volume — закрываемые лоты
    // Возвращает положительное значение (расход).
    [[nodiscard]] double closeCommission(double volume) const noexcept;
    // Комиссия за открытие (симметрично).
    [[nodiscard]] double openCommission(double volume) const noexcept;
    // Полная круглая комиссия (open + close) за объём.
    [[nodiscard]] double roundCommission(double volume) const noexcept;
    const CommissionConfig& config() const noexcept { return cfg_; }
private:
    CommissionConfig cfg_;
};
} // namespace spartak::position