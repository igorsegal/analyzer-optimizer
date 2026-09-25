// =============================================================================
//  SPARTAK :: context/StructureValidatorHHHL.h
//  Проверка структуры HH/HL (Bullish) или LL/LH (Bearish)
//  по последовательностям High- и Low-экстремумов.
//
//  Логика:
//    Bullish: каждый следующий High > предыдущего
//             И каждый следующий Low  > предыдущего
//    Bearish: каждый следующий Low  < предыдущего
//             И каждый следующий High < предыдущего
//
//  Смотрим `lookback` последних экстремумов каждого типа.
//  Если данных меньше — Undefined.
// =============================================================================
#pragma once
#include "context/FractalPointDetector.h"
#include <vector>
#include <cstddef>
namespace spartak::context {
// -----------------------------------------------------------------------------
// Результат проверки структуры.
// -----------------------------------------------------------------------------
enum class StructureType {
    Undefined,
    Bullish,   // HH + HL
    Bearish    // LL + LH
};
// -----------------------------------------------------------------------------
// StructureValidatorHHHL — stateless.
// -----------------------------------------------------------------------------
class StructureValidatorHHHL {
public:
    // Смотрим `lookback` последних High и Low.
    // lookback >= 2 — иначе проверять нечего.
    [[nodiscard]] static StructureType
    validate(const std::vector<Extremum>& highs,
             const std::vector<Extremum>& lows,
             std::size_t lookback);
    // Удобные обёртки
    [[nodiscard]] static bool isBullish(const std::vector<Extremum>& highs,
                                        const std::vector<Extremum>& lows,
                                        std::size_t lookback);
    [[nodiscard]] static bool isBearish(const std::vector<Extremum>& highs,
                                        const std::vector<Extremum>& lows,
                                        std::size_t lookback);
};
} // namespace spartak::context