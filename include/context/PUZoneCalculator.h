// =============================================================================
//  SPARTAK :: context/PUZoneCalculator.h
//  Построение зон ПУ (Промежуточный Уровень) из экстремумов старшего ТФ.
//
//  Формула (из ТЗ «Зри в Корень»):
//    zone_top    = price_level + offset_points * point
//    zone_bottom = price_level - offset_points * point
//
//  Возвращает PriceZone с LevelType::IntermediateLevel.
// =============================================================================
#pragma once
#include "core/Types.h"
#include "context/FractalPointDetector.h"
#include <cstddef>
#include <vector>
namespace spartak::context {
class PUZoneCalculator {
public:
    explicit PUZoneCalculator(double point, int offset_points = 15);
    // Одна зона из одного экстремума.
    [[nodiscard]] core::PriceZone
    buildOne(const Extremum& ex) const;
    // Список зон: берём последние max_zones экстремумов.
    // Свежие экстремумы — первыми в результате.
    [[nodiscard]] std::vector<core::PriceZone>
    build(const std::vector<Extremum>& extrema, std::size_t max_zones) const;
    double point()        const noexcept { return point_; }
    int    offsetPoints() const noexcept { return offset_points_; }
private:
    double point_;
    int    offset_points_;
};
} // namespace spartak::context