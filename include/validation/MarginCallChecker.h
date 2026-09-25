// =============================================================================
//  SPARTAK :: validation/MarginCallChecker.h
//  Проверка свободной маржи и уровня маржи (Margin Level).
//
//  Институциональный лимит:
//    MarginLevel = (Equity / TotalMargin) * 100 >= min_margin_level_pct
//
//  Проверяем ДО открытия ордера:
//    NewMargin    = (Volume * ContractSize) / Leverage
//    TotalMargin  = margin_used + NewMargin
//    MarginLevel  = (Equity / TotalMargin) * 100
//
//  Если MarginLevel < min_margin_level_pct -> REJECT (MarginCall).
//  Дополнительно проверяем: NewMargin <= free_margin.
// =============================================================================
#pragma once
#include <cstdint>
namespace spartak::validation {
// -----------------------------------------------------------------------------
// Результат проверки.
// -----------------------------------------------------------------------------
enum class MarginCheckResult {
    Ok,
    NotEnoughFreeMargin,   // new_margin > free_margin
    MarginCall             // level < min_margin_level_pct
};
[[nodiscard]] const char* to_string(MarginCheckResult r) noexcept;
// -----------------------------------------------------------------------------
// Состояние счёта (то, что нужно для проверки).
// -----------------------------------------------------------------------------
struct MarginAccountState {
    double equity                = 10'000.0;
    double free_margin           = 10'000.0;
    double margin_used           = 0.0;
    double leverage              = 500.0;
    double min_margin_level_pct  = 5'000.0;
};
// -----------------------------------------------------------------------------
// Конфиг.
// -----------------------------------------------------------------------------
struct MarginConfig {
    double contract_size = 100'000.0;
};
// -----------------------------------------------------------------------------
// MarginCallChecker — stateless.
// -----------------------------------------------------------------------------
class MarginCallChecker {
public:
    explicit MarginCallChecker(MarginConfig cfg = {});
    // Залог под ордер: (volume * contract_size) / leverage.
    [[nodiscard]] double calcMargin(double volume,
                                    double leverage) const noexcept;
    // Уровень маржи: (equity / total_margin) * 100.
    [[nodiscard]] double calcMarginLevelPct(double equity,
                                            double total_margin) const noexcept;
    // Полная проверка.
    [[nodiscard]] MarginCheckResult check(
        double volume,
        const MarginAccountState& acc) const noexcept;
    const MarginConfig& config() const noexcept { return cfg_; }
private:
    MarginConfig cfg_;
};
} // namespace spartak::validation