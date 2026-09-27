// =============================================================================
//  SPARTAK :: engine/PnlCalculator.h
//  Единая точка расчёта PnL. Результат ВСЕГДА в USD.
//
//  Формула:
//    gross_quote  = (exit - entry) * dir * volume * contract_size
//    factor       = quoteToUsdFactor(spec, exit_price)
//    gross_usd    = gross_quote * factor
//
//  где dir = +1 для BUY, -1 для SELL.
//
//  Все, кто считает PnL (PositionManager, Strategy и т.д.), обязаны
//  использовать ТОЛЬКО этот класс. Тогда любая сделка возвращает деньги
//  в USD, независимо от инструмента.
// =============================================================================
#pragma once
#include "core/Types.h"
#include "core/InstrumentSpec.h"
#include "engine/QuoteRateProvider.h"
namespace spartak::engine {
class PnlCalculator {
public:
    explicit PnlCalculator(const QuoteRateProvider& rates = QuoteRateProvider::makeDefault())
        : rates_(rates) {}
    // Gross PnL в USD, без комиссий и свопов.
    double grossPnlUsd(core::OrderSide side,
                       double entry_price,
                       double exit_price,
                       double volume,
                       const core::InstrumentSpec& spec) const noexcept;
    // Net PnL в USD: gross - commission - swap.
    // commission и swap должны быть уже в USD (брокер присылает в USD).
    double netPnlUsd(core::OrderSide side,
                     double entry_price,
                     double exit_price,
                     double volume,
                     double commission_usd,
                     double swap_usd,
                     const core::InstrumentSpec& spec) const noexcept;
    const QuoteRateProvider& rates() const noexcept { return rates_; }
private:
    QuoteRateProvider rates_;
    static double dir(core::OrderSide side) noexcept {
        return (side == core::OrderSide::Buy) ? 1.0 : -1.0;
    }
};
} // namespace spartak::engine