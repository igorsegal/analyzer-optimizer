#include "context/StructureValidatorHHHL.h"
namespace spartak::context {
// -----------------------------------------------------------------------------
// validate — основная логика.
//
// Алгоритм:
//   1. Нужно минимум lookback High и lookback Low. Иначе Undefined.
//   2. Берём последние lookback каждого типа.
//   3. Идём по парам соседей:
//        hh = все h[i] > h[i-1]
//        hl = все l[i] > l[i-1]
//        ll = все l[i] < l[i-1]
//        lh = все h[i] < h[i-1]
//   4. Bullish — hh && hl  (и НЕ (ll && lh) — на случай противоречия)
//      Bearish — ll && lh  (и НЕ (hh && hl))
// -----------------------------------------------------------------------------
StructureType StructureValidatorHHHL::validate(
        const std::vector<Extremum>& highs,
        const std::vector<Extremum>& lows,
        std::size_t lookback)
{
    if (lookback < 2) return StructureType::Undefined;
    if (highs.size() < lookback) return StructureType::Undefined;
    if (lows.size()  < lookback) return StructureType::Undefined;
    const std::size_t h0 = highs.size() - lookback;
    const std::size_t l0 = lows.size()  - lookback;
    bool hh = true;   // higher highs
    bool hl = true;   // higher lows
    bool ll = true;   // lower lows
    bool lh = true;   // lower highs
    for (std::size_t k = 1; k < lookback; ++k) {
        const double h_prev = highs[h0 + k - 1].price;
        const double h_cur  = highs[h0 + k].price;
        const double l_prev = lows [l0 + k - 1].price;
        const double l_cur  = lows [l0 + k].price;
        if (!(h_cur > h_prev)) hh = false;
        if (!(l_cur > l_prev)) hl = false;
        if (!(l_cur < l_prev)) ll = false;
        if (!(h_cur < h_prev)) lh = false;
        // Ранний выход, если уже всё опровергнуто
        if (!hh && !hl && !ll && !lh) break;
    }
    const bool bullish =  hh &&  hl && !(ll && lh);
    const bool bearish =  ll &&  lh && !(hh && hl);
    if (bullish) return StructureType::Bullish;
    if (bearish) return StructureType::Bearish;
    return StructureType::Undefined;
}
// -----------------------------------------------------------------------------
// Удобные обёртки
// -----------------------------------------------------------------------------
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