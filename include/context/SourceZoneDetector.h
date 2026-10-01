// =============================================================================
//  SPARTAK :: context/SourceZoneDetector.h
//  Детектор источника тренда (ТЗ ч.4, ч.5).
//
//  Источник = зона консолидации, из которой цена вышла импульсом и больше
//  не возвращалась. ОРТ (область разворота тенденции) = зона ЗА источником.
//
//  Алгоритм (упрощённый по курсу):
//    1. Скользящее окно [min_bars..max_bars] баров ищет узкий диапазон
//       (max_high - min_low <= max_range_points).
//    2. После окна ищет импульсный выход: цена ушла за границу на
//       breakout_points и не вернулась в течение confirm_bars.
//    3. Определяет направление (вверх = Bullish, вниз = Bearish).
//    4. Возвращает зону: top, bottom, level (граница пробития).
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstddef>
#include <vector>
namespace spartak::context {
struct SourceZone {
    bool                   found = false;
    double                 top = 0.0;
    double                 bottom = 0.0;
    double                 level = 0.0;      // уровень пробития
    std::size_t            start_index = 0;  // начало зоны в bars
    std::size_t            end_index = 0;    // конец зоны
    core::TrendDirection   direction = core::TrendDirection::Undefined;
};
struct SourceZoneConfig {
    std::size_t min_bars_in_zone = 15;    // минимум баров в консолидации
    std::size_t max_bars_in_zone = 80;    // максимум баров
    int         max_range_points = 2000;  // макс. разброс в зоне (200 пипсов)
    int         breakout_points  = 500;   // мин. выход за границу (50 пипсов)
    int         confirm_bars     = 5;     // сколько баров без возврата
    double      point            = 0.00001;
};
class SourceZoneDetector {
public:
    explicit SourceZoneDetector(SourceZoneConfig cfg = {});
    // Ищет последний (самый свежий) источник в переданной истории.
    [[nodiscard]] SourceZone find(const std::vector<core::Bar>& bars) const;
    const SourceZoneConfig& config() const noexcept { return cfg_; }
private:
    SourceZoneConfig cfg_;
};
} // namespace spartak::context