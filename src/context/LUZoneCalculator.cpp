#include "context/LUZoneCalculator.h"
#include <algorithm>
#include <stdexcept>
namespace spartak::context {
LUZoneCalculator::LUZoneCalculator(double point, int offset_points)
    : point_(point), offset_points_(offset_points) {
    if (point_ <= 0.0)
        throw std::invalid_argument("LUZoneCalculator: point must be > 0");
    if (offset_points_ < 0)
        throw std::invalid_argument("LUZoneCalculator: offset_points must be >= 0");
}
core::PriceZone LUZoneCalculator::buildOne(const Extremum& ex) const {
    const double delta = static_cast<double>(offset_points_) * point_;
    core::PriceZone z;
    z.type           = core::LevelType::LocalLevel;
    z.formation_time = ex.timestamp;
    z.price_level  = ex.price;
    z.zone_top     = ex.price + delta;
    z.zone_bottom  = ex.price - delta;
    z.is_active    = true;
    return z;
}
std::vector<core::PriceZone>
LUZoneCalculator::build(const std::vector<Extremum>& extrema,
                        std::size_t max_zones) const
{
    std::vector<core::PriceZone> out;
    if (extrema.empty() || max_zones == 0) return out;
    const std::size_t take = std::min(max_zones, extrema.size());
    out.reserve(take);
    for (std::size_t k = 0; k < take; ++k) {
        out.push_back(buildOne(extrema[extrema.size() - 1 - k]));
    }
    return out;
}
} // namespace spartak::context