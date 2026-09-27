#include "engine/PnlCalculator.h"
namespace spartak::engine {
double PnlCalculator::grossPnlUsd(core::OrderSide side,
                                  double entry_price,
                                  double exit_price,
                                  double volume,
                                  const core::InstrumentSpec& spec) const noexcept {
    if (volume <= 0.0) return 0.0;
    const double delta = (exit_price - entry_price) * dir(side);
    const double gross_quote = delta * volume * spec.contract_size;
    // Для xxx/USD factor = 1.0
    // Для USD/xxx factor = 1/exit_price (курс меняется, но используем exit)
    // Для cross factor из таблицы
    const double factor = rates_.quoteToUsdFactor(spec, exit_price);
    return gross_quote * factor;
}
double PnlCalculator::netPnlUsd(core::OrderSide side,
                                double entry_price,
                                double exit_price,
                                double volume,
                                double commission_usd,
                                double swap_usd,
                                const core::InstrumentSpec& spec) const noexcept {
    return grossPnlUsd(side, entry_price, exit_price, volume, spec)
         - commission_usd
         - swap_usd;
}
} // namespace spartak::engine