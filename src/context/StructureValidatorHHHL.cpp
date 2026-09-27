#include "context/StructureValidatorHHHL.h"
namespace spartak::context {
// -----------------------------------------------------------------------------
// validate — правило ТЗ: High_i > High_{i-2}, Low_i > Low_{i-2}
//
// Сравниваем КАЖДЫЙ последующий экстремум с экстремумом ЧЕРЕЗ ОДИН.
// lookback = число последних экстремумов для анализа (мин. 3).
//
//   5 экстремумов: h0 h1 h2 h3 h4
//     h2 > h0 ?
//     h3 > h1 ?
//     h4 > h2 ?
//   Все true -> Bullish
// -----------------------------------------------------------------------------
StructureType StructureValidatorHHHL::validate(
        const std::vector<Extremum>& highs,
        const std::vector<Extremum>& lows,
        std::size_t lookback)
{
    if (lookback < 3) return StructureType::Undefined;
    if (highs.size() < lookback) return StructureType::Undefined;
    if (lows.size()  < lookback) return StructureType::Undefined;
    const std::size_t h0 = highs.size() - lookback;
    const std::size_t l0 = lows.size()  - lookback;
    bool hh = true;   // higher highs (через один)
    bool hl = true;   // higher lows  (через один)
    bool ll = true;   // lower lows    (через один)
    bool lh = true;   // lower highs   (через один)
    for (std::size_t k = 1; k < lookback; ++k) {
        const double h_cur  = highs[h0 + k].price;
        const double h_prev2 = highs[h0 + k - 1].price;
        const double l_cur  = lows[l0 + k].price;
        const double l_prev2 = lows[l0 + k - 1].price;
        if (!(h_cur > h_prev2)) hh = false;
        if (!(l_cur > l_prev2)) hl = false;
        if (!(l_cur < l_prev2)) ll = false;
        if (!(h_cur < h_prev2)) lh = false;
        if (!hh && !hl && !ll && !lh) break;
    }
    const bool bullish =  hh &&  hl && !(ll && lh);
    const bool bearish =  ll &&  lh && !(hh && hl);
    if (bullish) return StructureType::Bullish;
    if (bearish) return StructureType::Bearish;
    return StructureType::Undefined;
}
bool StructureValidatorHHHL::isBullish(
        const std::vector<Extremum>& highs,
        const std::vector<Extremum>& lows,
        std::size_t lookback)
{
    return validate(highs, lows, lookback) == StructureType::Bullish;
}
bool StructureValidatorHHHL::isBearish(
        const std::vector<Extremum>& highs,
        const std::vector<Extremum>& lows,
        std::size_t lookback)
{
    return validate(highs, lows, lookback) == StructureType::Bearish;
}
} // namespace spartak::context