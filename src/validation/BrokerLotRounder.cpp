#include "validation/BrokerLotRounder.h"
#include <cmath>
#include <stdexcept>
namespace spartak::validation {
BrokerLotRounder::BrokerLotRounder(LotRounderConfig cfg)
    : cfg_(cfg) {
    if (cfg_.min_lot <= 0.0)
        throw std::invalid_argument("LotRounderConfig::min_lot must be > 0");
    if (cfg_.max_lot < cfg_.min_lot)
        throw std::invalid_argument("LotRounderConfig::max_lot must be >= min_lot");
    if (cfg_.lot_step <= 0.0)
        throw std::invalid_argument("LotRounderConfig::lot_step must be > 0");
}
// -----------------------------------------------------------------------------
// steps_in — сколько целых шагов в raw_lot.
// -----------------------------------------------------------------------------
std::int64_t BrokerLotRounder::steps_in(double raw_lot) const noexcept {
    if (raw_lot <= 0.0) return 0;
    return static_cast<std::int64_t>(std::floor((raw_lot + 1e-9) / cfg_.lot_step));
}
// -----------------------------------------------------------------------------
// round_down — floor до шага, обрезка по min/max, чистка floating-point мусора.
// -----------------------------------------------------------------------------
double BrokerLotRounder::round_down(double raw_lot) const noexcept {
    if (raw_lot <= 0.0) return 0.0;
    const std::int64_t steps = steps_in(raw_lot);
    // Ограничение сверху по max_lot
    const std::int64_t max_steps = static_cast<std::int64_t>(
        std::floor(cfg_.max_lot / cfg_.lot_step + 1e-9));
    const std::int64_t use_steps = (steps > max_steps) ? max_steps : steps;
    double v = static_cast<double>(use_steps) * cfg_.lot_step;
    // Убираем floating-point мусор (0.30000000000004 -> 0.3)
    v = std::round(v * 1e8) / 1e8;
    if (v + 1e-9 < cfg_.min_lot) return 0.0;
    return v;
}
// -----------------------------------------------------------------------------
// round_up — ceil до шага.
// -----------------------------------------------------------------------------
double BrokerLotRounder::round_up(double raw_lot) const noexcept {
    if (raw_lot <= 0.0) return 0.0;
    const std::int64_t steps = static_cast<std::int64_t>(
        std::ceil((raw_lot - 1e-9) / cfg_.lot_step));
    const std::int64_t max_steps = static_cast<std::int64_t>(
        std::floor(cfg_.max_lot / cfg_.lot_step + 1e-9));
    const std::int64_t use_steps = (steps > max_steps) ? max_steps : steps;
    double v = static_cast<double>(use_steps) * cfg_.lot_step;
    v = std::round(v * 1e8) / 1e8;
    if (v + 1e-9 < cfg_.min_lot) return 0.0;
    return v;
}
// -----------------------------------------------------------------------------
// is_valid_lot — лежит ли значение на шаге.
// -----------------------------------------------------------------------------
bool BrokerLotRounder::is_valid_lot(double lot) const noexcept {
    if (lot <= 0.0) return false;
    if (lot + 1e-9 < cfg_.min_lot) return false;
    if (lot - 1e-9 > cfg_.max_lot) return false;
    const double steps = lot / cfg_.lot_step;
    return std::fabs(steps - std::round(steps)) < 1e-6;
}
} // namespace spartak::validation