#include "context/TrendBiasEvaluator.h"
#include "context/StructureValidatorHHHL.h"
#include <stdexcept>
namespace spartak::context {
TrendBiasEvaluator::TrendBiasEvaluator(std::size_t lookback)
    : lookback_(lookback) {
    if (lookback_ < 2)
        throw std::invalid_argument("TrendBiasEvaluator: lookback must be >= 2");
}
// -----------------------------------------------------------------------------
// evaluate — определяет доминирующий тренд.
//
// Схема:
//   1. Прогоняем StructureValidatorHHHL на Daily и на H1.
//   2. Сравниваем результаты:
//      - оба Bullish  -> Bullish, both_agree=true
//      - оба Bearish  -> Bearish, both_agree=true
//      - один определён, другой Undefined -> берём определённый, fallback=true
//      - оба Undefined -> Undefined
//      - Bullish vs Bearish -> Undefined, conflict=true
// -----------------------------------------------------------------------------
TrendBias TrendBiasEvaluator::evaluate(
        const std::vector<Extremum>& daily_highs,
        const std::vector<Extremum>& daily_lows,
        const std::vector<Extremum>& hourly_highs,
        const std::vector<Extremum>& hourly_lows) const
{
    const auto d = StructureValidatorHHHL::validate(daily_highs, daily_lows, lookback_);
    const auto h = StructureValidatorHHHL::validate(hourly_highs, hourly_lows, lookback_);
    TrendBias bias;
    const bool d_bull = (d == StructureType::Bullish);
    const bool d_bear = (d == StructureType::Bearish);
    const bool h_bull = (h == StructureType::Bullish);
    const bool h_bear = (h == StructureType::Bearish);
    // Случай 1: оба определились и совпали
    if (d_bull && h_bull) {
        bias.direction = core::TrendDirection::Bullish;
        bias.both_agree = true;
        return bias;
    }
    if (d_bear && h_bear) {
        bias.direction = core::TrendDirection::Bearish;
        bias.both_agree = true;
        return bias;
    }
    // Случай 2: конфликт (Daily противоречит H1)
    if ((d_bull && h_bear) || (d_bear && h_bull)) {
        bias.direction = core::TrendDirection::Undefined;
        bias.conflict  = true;
        return bias;
    }
    // Случай 3: Daily определён, H1 — нет. Берём Daily.
    if (d_bull) {
        bias.direction = core::TrendDirection::Bullish;
        bias.fallback  = true;
        return bias;
    }
    if (d_bear) {
        bias.direction = core::TrendDirection::Bearish;
        bias.fallback  = true;
        return bias;
    }
    // Случай 4: H1 определён, Daily — нет. Берём H1.
    if (h_bull) {
        bias.direction = core::TrendDirection::Bullish;
        bias.fallback  = true;
        return bias;
    }
    if (h_bear) {
        bias.direction = core::TrendDirection::Bearish;
        bias.fallback  = true;
        return bias;
    }
    // Случай 5: оба Undefined
    return bias;
}
} // namespace spartak::context