#include "validation/MarginCallChecker.h"
#include <stdexcept>
namespace spartak::validation {
const char* to_string(MarginCheckResult r) noexcept {
    switch (r) {
        case MarginCheckResult::Ok:                  return "Ok";
        case MarginCheckResult::NotEnoughFreeMargin: return "NotEnoughFreeMargin";
        case MarginCheckResult::MarginCall:          return "MarginCall";
    }
    return "Unknown";
}
MarginCallChecker::MarginCallChecker(MarginConfig cfg)
    : cfg_(cfg) {
    if (cfg_.contract_size <= 0.0)
        throw std::invalid_argument("MarginConfig::contract_size must be > 0");
}
// -----------------------------------------------------------------------------
// calcMargin
// -----------------------------------------------------------------------------
double MarginCallChecker::calcMargin(double volume,
                                     double leverage) const noexcept {
    if (leverage <= 0.0) return 1e18;
    if (volume <= 0.0)   return 0.0;
    return (volume * cfg_.contract_size) / leverage;
}
// -----------------------------------------------------------------------------
// calcMarginLevelPct
// -----------------------------------------------------------------------------
double MarginCallChecker::calcMarginLevelPct(double equity,
                                             double total_margin) const noexcept {
    if (total_margin <= 0.0) return 1e9;   // условная «бесконечность»
    return (equity / total_margin) * 100.0;
}
// -----------------------------------------------------------------------------
// check — полная логика.
//
//   1. Считаем new_margin под ордер.
//   2. Проверяем: new_margin <= free_margin. Иначе NotEnoughFreeMargin.
//   3. TotalMargin = margin_used + new_margin.
//   4. MarginLevel = equity / total * 100.
//   5. Если level < min_margin_level_pct -> MarginCall.
//   6. Иначе Ok.
// -----------------------------------------------------------------------------
MarginCheckResult MarginCallChecker::check(
        double volume,
        const MarginAccountState& acc) const noexcept
{
    if (volume <= 0.0) return MarginCheckResult::Ok;
    const double new_margin = calcMargin(volume, acc.leverage);
    // 1. Свободная маржа
    if (new_margin > acc.free_margin + 1e-9) {
        return MarginCheckResult::NotEnoughFreeMargin;
    }
    // 2. Уровень маржи после открытия
    const double total_margin = acc.margin_used + new_margin;
    const double level_pct    = calcMarginLevelPct(acc.equity, total_margin);
    if (level_pct < acc.min_margin_level_pct) {
        return MarginCheckResult::MarginCall;
    }
    return MarginCheckResult::Ok;
}
} // namespace spartak::validation