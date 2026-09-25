#include "context/PUZoneCalculator.h"
#include <algorithm>
#include <stdexcept>
namespace spartak::context {
// -----------------------------------------------------------------------------
// Конструктор с валидацией.
// -----------------------------------------------------------------------------
PUZoneCalculator::PUZoneCalculator(double point, int offset_points)
    : point_(point), offset_points_(offset_points) {
    if (point_ <= 0.0)
        throw std::invalid_argument("PUZoneCalculator: point must be > 0");
    if (offset_points_ < 0)
        throw std::invalid_argument("PUZoneCalculator: offset_points must be >= 0");
}
// -----------------------------------------------------------------------------
// buildOne — одна зона из экстремума.
// -----------------------------------------------------------------------------
core::PriceZone PUZoneCalculator::buildOne(const Extremum& ex) const {
    const double delta = static_cast<double>(offset_points_) * point_;
    core::PriceZone z;
    z.type         = core::LevelType::IntermediateLevel;
    z.price_level  = ex.price;
    z.zone_top     = ex.price + delta;
    z.zone_bottom  = ex.price - delta;
    z.is_active    = true;
    return z;
}
// -----------------------------------------------------------------------------
// build — список зон из последних max_zones экстремумов.
// Свежие экстремумы идут первыми в результате.
// -----------------------------------------------------------------------------
std::vector<core::PriceZone>
PUZoneCalculator::build(const std::vector<Extremum>& extrema,
                        std::size_t max_zones) const
{
    std::vector<core::PriceZone> out;
    if (extrema.empty() || max_zones == 0) return out;
    const std::size_t take = std::min(max_zones, extrema.size());
    out.reserve(take);
    // Идём с конца: свежие экстремумы важнее старых.
    for (std::size_t k = 0; k < take; ++k) {
        out.push_back(buildOne(extrema[extrema.size() - 1 - k]));
    }
    return out;
}
} // namespace spartak::context