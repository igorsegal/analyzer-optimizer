#include "spartak/strategy.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace spartak {

void MarketBar::validate() const {
    const auto finite = [](const double value) { return std::isfinite(value); };
    if (!finite(open) || !finite(high) || !finite(low) || !finite(close)) {
        throw std::invalid_argument("market bar contains a non-finite OHLC value");
    }
    if (open <= 0.0 || high <= 0.0 || low <= 0.0 || close <= 0.0) {
        throw std::invalid_argument("market bar prices must be positive");
    }
    if (high < std::max(open, close) || low > std::min(open, close) || low > high) {
        throw std::invalid_argument("market bar violates OHLC ordering");
    }
    if (bid_ask_spread && (!finite(*bid_ask_spread) || *bid_ask_spread < 0.0)) {
        throw std::invalid_argument("bar spread must be finite and non-negative");
    }
}

void StrategyParameters::validate_for_execution() const {
    if (source_pending) {
        throw std::logic_error(
            "strategy rules are source_pending; execution is intentionally blocked");
    }
    if (rule_revision.empty() || rule_revision == "source_pending") {
        throw std::invalid_argument("an executable strategy needs a locked rule revision");
    }
    for (const auto& [name, value] : numeric) {
        if (name.empty() || !std::isfinite(value)) {
            throw std::invalid_argument("strategy numeric parameters must have names and finite values");
        }
    }
    for (const auto& [name, value] : boolean) {
        (void)value;
        if (name.empty()) {
            throw std::invalid_argument("strategy boolean parameters must have names");
        }
    }
}

double StrategyParameters::require_numeric(const std::string_view name) const {
    const auto found = numeric.find(name);
    if (found == numeric.end()) {
        throw std::out_of_range("missing numeric strategy parameter: " + std::string{name});
    }
    return found->second;
}

bool StrategyParameters::require_boolean(const std::string_view name) const {
    const auto found = boolean.find(name);
    if (found == boolean.end()) {
        throw std::out_of_range("missing boolean strategy parameter: " + std::string{name});
    }
    return found->second;
}

std::optional<TradeSignal> SourcePendingStrategy::on_bar_close(
    const std::span<const MarketBar> bars,
    const std::size_t bar_index,
    const StrategyParameters& parameters) const {
    (void)bars;
    (void)bar_index;
    (void)parameters;
    throw std::logic_error(
        "Spartak strategy implementation is source_pending; no rules were inferred");
}

std::string canonical_parameter_key(const StrategyParameters& parameters) {
    std::ostringstream output;
    output << "pending=" << (parameters.source_pending ? '1' : '0')
           << ";revision=" << parameters.rule_revision;
    output << std::setprecision(std::numeric_limits<double>::max_digits10);
    for (const auto& [name, value] : parameters.numeric) {
        output << ";n:" << name << '=' << value;
    }
    for (const auto& [name, value] : parameters.boolean) {
        output << ";b:" << name << '=' << (value ? '1' : '0');
    }
    return output.str();
}

}  // namespace spartak
