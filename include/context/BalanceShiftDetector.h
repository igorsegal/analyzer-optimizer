// =============================================================================
//  SPARTAK :: context/BalanceShiftDetector.h
//  Детектор перевеса / перелива (ТЗ ч.7).
//
//  Идея автора:
//    Перевес — это момент, когда деньги (позиции) перетекли в одну сторону
//    баланса. Определяется ПО ТАЙМИНГУ ФОРМАЦИИ, а не по индикатору объёма.
//
//  Упрощённая формализация:
//    - ищем две соседние зоны-источника (SourceZone);
//    - если направление второй совпадает с первой -> перевес усиливается;
//    - если второе направление противоположно -> возможен разворот.
//
//  Модуль нужен как аналитический слой: не торгует, а даёт сигнал «куда
//  сейчас перевес» для последующих решений.
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstddef>
#include <vector>
namespace spartak::context {
struct SourceZone;   // из SourceZoneDetector.h
// -----------------------------------------------------------------------------
// Направление перевеса.
// -----------------------------------------------------------------------------
enum class BalanceSide {
    None = 0,        // перевеса нет
    Bullish,         // деньги перетекли в покупки
    Bearish          // деньги перетекли в продажи
};
struct BalanceShift {
    bool         found = false;
    BalanceSide  side = BalanceSide::None;
    double       strength = 0.0;   // 0..1, для диагностики
    std::size_t  first_zone_end = 0;
    std::size_t  second_zone_end = 0;
};
// -----------------------------------------------------------------------------
// Конфиг.
// -----------------------------------------------------------------------------
struct BalanceShiftConfig {
    std::size_t min_gap_bars = 5;    // мин. разрыв между зонами
    std::size_t max_gap_bars = 500;  // макс. разрыв
    double      point        = 0.00001;
};
// -----------------------------------------------------------------------------
// BalanceShiftDetector — stateless.
// -----------------------------------------------------------------------------
class BalanceShiftDetector {
public:
    explicit BalanceShiftDetector(BalanceShiftConfig cfg = {});
    // Принимает список зон-источников в хронологическом порядке
    // (получается в ContextAggregator при анализе истории).
    [[nodiscard]] BalanceShift detect(
        const std::vector<SourceZone>& zones) const;
    const BalanceShiftConfig& config() const noexcept { return cfg_; }
private:
    BalanceShiftConfig cfg_;
};
} // namespace spartak::context