// =============================================================================
//  SPARTAK :: context/LUZoneCalculator.h
//  Построение зон ЛУ (Локальный Уровень) из экстремумов младшего ТФ (H1).
//
//  Формула идентична PUZoneCalculator, но результат маркируется
//  LevelType::LocalLevel. ЛУ используется для точечного удержания цены
//  и контр-трендовых входов.
// =============================================================================
#pragma once
#include "core/Types.h"
#include "context/FractalPointDetector.h"
#include <cstddef>
#include <vector>
namespace spartak::context {
class LUZoneCalculator {
public:
    explicit LUZoneCalculator(double point, int offset_points = 15);
    [[nodiscard]] core::PriceZone
    buildOne(const Extremum& ex) const;
    [[nodiscard]] std::vector<core::PriceZone>
    build(const std::vector<Extremum>& extrema, std::size_t max_zones) const;
    double point()        const noexcept { return point_; }
    int    offsetPoints() const noexcept { return offset_points_; }
private:
    double point_;
    int    offset_points_;
};
} // namespace spartak::context