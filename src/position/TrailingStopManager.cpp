#include "position/TrailingStopManager.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace spartak::position {
TrailingStopManager::TrailingStopManager(TrailingConfig cfg)
    : cfg_(cfg) {
    if (cfg_.trailing_distance_points <= 0)
        throw std::invalid_argument("TrailingConfig::trailing_distance_points must be > 0");
    if (cfg_.activation_points < 0)
        throw std::invalid_argument("TrailingConfig::activation_points must be >= 0");
    if (cfg_.min_step_points < 0)
        throw std::invalid_argument("TrailingConfig::min_step_points must be >= 0");
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("TrailingConfig::point must be > 0");
}
TrailingResult TrailingStopManager::compute(core::OrderSide side,
                                            double current_sl,
                                            double entry,
                                            const core::Bar& bar) const noexcept
{
    TrailingResult r;
    r.new_sl = current_sl;
    if (entry <= 0.0 || current_sl <= 0.0) return r;
    const double trail_dist = static_cast<double>(cfg_.trailing_distance_points) * cfg_.point;
    const double act_dist   = static_cast<double>(cfg_.activation_points) * cfg_.point;
    const double min_step   = static_cast<double>(cfg_.min_step_points) * cfg_.point;
    if (side == core::OrderSide::Buy) {
        // Activation: только если цена ушла в прибыль на act_dist
        if (cfg_.activation_points > 0 && bar.high < entry + act_dist) {
            return r;
        }
        const double candidate = bar.high - trail_dist;
        // Только вверх (улучшение стопа)
        if (candidate > current_sl + min_step - 1e-12) {
            r.moved    = true;
            r.new_sl   = candidate;
            r.move_pts = (candidate - current_sl) / cfg_.point;
        }
        return r;
    }
    // SELL
    if (cfg_.activation_points > 0 && bar.low > entry - act_dist) {
        return r;
    }
    const double candidate = bar.low + trail_dist;
    // Только вниз (улучшение стопа для SELL)
    if (candidate < current_sl - min_step + 1e-12) {
        r.moved    = true;
        r.new_sl   = candidate;
        r.move_pts = (current_sl - candidate) / cfg_.point;
    }
    return r;
}
} // namespace spartak::position