#pragma once

#include "spartak/strategy.hpp"

namespace spartak {

// A causal, single-series machine hypothesis derived from the documented
// sequence accumulation -> breakout -> retest -> confirmation. It is not a
// claim that the numeric thresholds are original Spartak rules.
class SpartakPriceBaselineStrategy final : public IStrategy {
public:
    [[nodiscard]] std::optional<TradeSignal> on_bar_close(
        std::span<const MarketBar> bars,
        std::size_t bar_index,
        const StrategyParameters& parameters) const override;
};

}  // namespace spartak
