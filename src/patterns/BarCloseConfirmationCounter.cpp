#include "patterns/BarCloseConfirmationCounter.h"
namespace spartak::patterns {
// -----------------------------------------------------------------------------
// countAbove — идём с конца, считаем подряд close > level.
// Первый же бар с close <= level обрывает счёт.
// -----------------------------------------------------------------------------
std::size_t BarCloseConfirmationCounter::countAbove(
        const std::vector<core::Bar>& bars, double level) noexcept
{
    std::size_t n = 0;
    for (auto it = bars.rbegin(); it != bars.rend(); ++it) {
        if (it->close > level) ++n;
        else break;
    }
    return n;
}
// -----------------------------------------------------------------------------
// countBelow — идём с конца, считаем подряд close < level.
// -----------------------------------------------------------------------------
std::size_t BarCloseConfirmationCounter::countBelow(
        const std::vector<core::Bar>& bars, double level) noexcept
{
    std::size_t n = 0;
    for (auto it = bars.rbegin(); it != bars.rend(); ++it) {
        if (it->close < level) ++n;
        else break;
    }
    return n;
}
// -----------------------------------------------------------------------------
// isConfirmedAbove — countAbove >= min_bars.
// -----------------------------------------------------------------------------
bool BarCloseConfirmationCounter::isConfirmedAbove(
        const std::vector<core::Bar>& bars,
        double level,
        std::size_t min_bars) noexcept
{
    if (min_bars == 0) return true;
    if (bars.size() < min_bars) return false;
    return countAbove(bars, level) >= min_bars;
}
// -----------------------------------------------------------------------------
// isConfirmedBelow — countBelow >= min_bars.
// -----------------------------------------------------------------------------
bool BarCloseConfirmationCounter::isConfirmedBelow(
        const std::vector<core::Bar>& bars,
        double level,
        std::size_t min_bars) noexcept
{
    if (min_bars == 0) return true;
    if (bars.size() < min_bars) return false;
    return countBelow(bars, level) >= min_bars;
}
} // namespace spartak::patterns