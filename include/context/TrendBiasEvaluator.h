// =============================================================================
//  SPARTAK :: context/TrendBiasEvaluator.h
//  Оценка доминирующего тренда по двум таймфреймам.
//
//  Логика:
//    Daily + H1 оба Bullish  -> Bullish
//    Daily + H1 оба Bearish  -> Bearish
//    Один определён, другой Undefined -> берём определённый (fallback)
//    Конфликт (Bullish vs Bearish)    -> Undefined
//
//  Использует StructureValidatorHHHL как базовый примитив.
// =============================================================================
#pragma once
#include "core/Types.h"
#include "context/FractalPointDetector.h"
#include <cstddef>
#include <vector>
namespace spartak::context {
// -----------------------------------------------------------------------------
// Расширенный результат: направление + уверенность.
// -----------------------------------------------------------------------------
struct TrendBias {
    core::TrendDirection direction  = core::TrendDirection::Undefined;
    bool                 both_agree = false;  // Daily и H1 совпали
    bool                 fallback   = false;  // использован только один ТФ
    bool                 conflict   = false;  // Daily != H1
};
// -----------------------------------------------------------------------------
// TrendBiasEvaluator — stateless.
// -----------------------------------------------------------------------------
class TrendBiasEvaluator {
public:
    explicit TrendBiasEvaluator(std::size_t lookback = 3);
    [[nodiscard]] TrendBias evaluate(const std::vector<Extremum>& daily_highs,
                                     const std::vector<Extremum>& daily_lows,
                                     const std::vector<Extremum>& hourly_highs,
                                     const std::vector<Extremum>& hourly_lows) const;
    std::size_t lookback() const noexcept { return lookback_; }
private:
    std::size_t lookback_;
};
} // namespace spartak::context