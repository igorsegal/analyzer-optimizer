// =============================================================================
//  SPARTAK :: patterns/BarCloseConfirmationCounter.h
//  Счётчик подтверждений закрытия свечей относительно уровня.
//
//  Задача: убедиться, что цена «закрепилась» над/под уровнем,
//  а не пробила его одним выстрелом и вернулась обратно.
//
//  Методы:
//    countAbove / countBelow — сколько последних подряд баров закрылись
//                              с нужной стороны уровня.
//    isConfirmedAbove/Below — проверка порога min_bars.
//
//  Используется в Consolidation-детекторе (2-3 бара удержания уровня).
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstddef>
#include <vector>
namespace spartak::patterns {
class BarCloseConfirmationCounter {
public:
    // Сколько последних подряд баров закрылись выше level
    // (по полю close).
    [[nodiscard]] static std::size_t
    countAbove(const std::vector<core::Bar>& bars, double level) noexcept;
    // То же, но ниже level.
    [[nodiscard]] static std::size_t
    countBelow(const std::vector<core::Bar>& bars, double level) noexcept;
    // Подтверждение: >= min_bars подряд закрытий с одной стороны.
    [[nodiscard]] static bool
    isConfirmedAbove(const std::vector<core::Bar>& bars,
                     double level,
                     std::size_t min_bars) noexcept;
    [[nodiscard]] static bool
    isConfirmedBelow(const std::vector<core::Bar>& bars,
                     double level,
                     std::size_t min_bars) noexcept;
};
} // namespace spartak::patterns