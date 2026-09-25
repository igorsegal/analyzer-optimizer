#include "position/BreakEvenTransfer.h"
#include <cmath>
#include <stdexcept>
namespace spartak::position {
BreakEvenTransfer::BreakEvenTransfer(BreakEvenConfig cfg)
    : cfg_(cfg) {
    if (cfg_.fallback_spread_points < 0)
        throw std::invalid_argument("BreakEvenConfig::fallback_spread_points must be >= 0");
    if (cfg_.extra_buffer_points < 0)
        throw std::invalid_argument("BreakEvenConfig::extra_buffer_points must be >= 0");
    if (cfg_.clamp_buffer_points < 0)
        throw std::invalid_argument("BreakEvenConfig::clamp_buffer_points must be >= 0");
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("BreakEvenConfig::point must be > 0");
}
BreakEvenResult BreakEvenTransfer::compute(core::OrderSide side,
                                           double entry,
                                           double tp1,
                                           int32_t spread_pts,
                                           double current_sl) const noexcept
{
    BreakEvenResult r;
    if (entry <= 0.0) return r;
    const int32_t spread_use = (spread_pts > 0) ? spread_pts
                                                : cfg_.fallback_spread_points;
    const double offset_pts =
        (cfg_.use_spread_in_be ? static_cast<double>(spread_use) : 0.0)
      + static_cast<double>(cfg_.extra_buffer_points);
    const double offset = offset_pts * cfg_.point;
    // Базовый BE-стоп
    double new_sl;
    if (side == core::OrderSide::Buy) {
        new_sl = entry + offset;
    } else {
        new_sl = entry - offset;
    }
    // Clamp: BE-стоп не должен перепрыгнуть TP1
    const double clamp_buf = static_cast<double>(cfg_.clamp_buffer_points) * cfg_.point;
    if (side == core::OrderSide::Buy && tp1 > 0.0 && new_sl >= tp1) {
        new_sl  = tp1 - clamp_buf;
        r.clamped = true;
    }
    if (side == core::OrderSide::Sell && tp1 > 0.0 && new_sl <= tp1) {
        new_sl  = tp1 + clamp_buf;
        r.clamped = true;
    }
    r.ok     = true;
    r.new_sl = new_sl;
    r.move_pts = std::fabs(new_sl - current_sl) / cfg_.point;
    return r;
}
} // namespace spartak::position