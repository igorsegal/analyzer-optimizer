#include "position/CloseCommissionCalculator.h"
#include <cmath>
#include <stdexcept>
namespace spartak::position {
CloseCommissionCalculator::CloseCommissionCalculator(CommissionConfig cfg)
    : cfg_(cfg) {
    if (cfg_.commission_per_lot < 0.0)
        throw std::invalid_argument("CommissionConfig::commission_per_lot must be >= 0");
}
// -----------------------------------------------------------------------------
// closeCommission — половина круговой комиссии за закрываемый объём.
// -----------------------------------------------------------------------------
double CloseCommissionCalculator::closeCommission(double volume) const noexcept {
    if (volume <= 0.0) return 0.0;
    return volume * cfg_.commission_per_lot * 0.5;
}
// -----------------------------------------------------------------------------
// openCommission — половина круговой комиссии за открываемый объём.
// -----------------------------------------------------------------------------
double CloseCommissionCalculator::openCommission(double volume) const noexcept {
    if (volume <= 0.0) return 0.0;
    return volume * cfg_.commission_per_lot * 0.5;
}
// -----------------------------------------------------------------------------
// roundCommission — полная комиссия (open + close).
// -----------------------------------------------------------------------------
double CloseCommissionCalculator::roundCommission(double volume) const noexcept {
    if (volume <= 0.0) return 0.0;
    return volume * cfg_.commission_per_lot;
}
} // namespace spartak::position