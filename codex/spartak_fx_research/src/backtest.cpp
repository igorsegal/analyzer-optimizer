#include "spartak/backtest.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace spartak {
namespace {

constexpr double kBpsScale = 1.0e-4;

[[nodiscard]] bool finite_non_negative(const double value) {
    return std::isfinite(value) && value >= 0.0;
}

[[nodiscard]] double order_fee(
    const FeeSchedule& fees,
    const double quantity,
    const double price) {
    return fees.fixed_per_order
        + std::abs(quantity * price) * fees.proportional_bps * kBpsScale;
}

[[nodiscard]] double adverse_fill(
    const PositionSide side,
    const bool entry,
    const double quote_price,
    const double slippage) {
    if (side == PositionSide::long_position) {
        return quote_price + (entry ? slippage : -slippage);
    }
    return quote_price + (entry ? -slippage : slippage);
}

struct PendingSignal {
    Timestamp signal_timestamp{};
    TradeSignal signal;
};

struct OpenPosition {
    Timestamp signal_timestamp{};
    Timestamp entry_timestamp{};
    PositionSide side{PositionSide::long_position};
    double risk_budget{};
    double quantity{};
    double entry_price{};
    double entry_fee{};
    double stop_loss{};
    std::optional<double> take_profit;
    std::string rule_id;
};

struct ExitDecision {
    double fill_price{};
    ExitReason reason{ExitReason::end_of_data};
    bool same_bar_ambiguity{};
};

[[nodiscard]] std::optional<ExitDecision> protective_exit(
    const OpenPosition& position,
    const MarketBar& bar,
    const double spread,
    const double slippage) {
    const double half_spread = spread * 0.5;

    if (position.side == PositionSide::long_position) {
        const double bid_open = bar.open - half_spread;
        const double bid_high = bar.high - half_spread;
        const double bid_low = bar.low - half_spread;

        if (bid_open <= position.stop_loss) {
            return ExitDecision{
                adverse_fill(position.side, false, bid_open, slippage),
                ExitReason::stop_loss,
                false};
        }
        if (position.take_profit && bid_open >= *position.take_profit) {
            return ExitDecision{
                adverse_fill(position.side, false, *position.take_profit, slippage),
                ExitReason::take_profit,
                false};
        }

        const bool stop_touched = bid_low <= position.stop_loss;
        const bool target_touched = position.take_profit && bid_high >= *position.take_profit;
        if (stop_touched) {
            return ExitDecision{
                adverse_fill(position.side, false, position.stop_loss, slippage),
                ExitReason::stop_loss,
                target_touched};
        }
        if (target_touched) {
            return ExitDecision{
                adverse_fill(position.side, false, *position.take_profit, slippage),
                ExitReason::take_profit,
                false};
        }
        return std::nullopt;
    }

    const double ask_open = bar.open + half_spread;
    const double ask_high = bar.high + half_spread;
    const double ask_low = bar.low + half_spread;

    if (ask_open >= position.stop_loss) {
        return ExitDecision{
            adverse_fill(position.side, false, ask_open, slippage),
            ExitReason::stop_loss,
            false};
    }
    if (position.take_profit && ask_open <= *position.take_profit) {
        return ExitDecision{
            adverse_fill(position.side, false, *position.take_profit, slippage),
            ExitReason::take_profit,
            false};
    }

    const bool stop_touched = ask_high >= position.stop_loss;
    const bool target_touched = position.take_profit && ask_low <= *position.take_profit;
    if (stop_touched) {
        return ExitDecision{
            adverse_fill(position.side, false, position.stop_loss, slippage),
            ExitReason::stop_loss,
            target_touched};
    }
    if (target_touched) {
        return ExitDecision{
            adverse_fill(position.side, false, *position.take_profit, slippage),
            ExitReason::take_profit,
            false};
    }
    return std::nullopt;
}

}  // namespace

void BacktestConfig::validate() const {
    if (!finite_non_negative(execution.default_spread)
        || !finite_non_negative(execution.slippage_per_fill)
        || !finite_non_negative(execution.fees.fixed_per_order)
        || !finite_non_negative(execution.fees.proportional_bps)) {
        throw std::invalid_argument("execution costs must be finite and non-negative");
    }
    if (!std::isfinite(risk.initial_equity) || risk.initial_equity <= 0.0) {
        throw std::invalid_argument("initial equity must be finite and positive");
    }
    if (!std::isfinite(risk.risk_fraction_per_trade)
        || risk.risk_fraction_per_trade <= 0.0
        || risk.risk_fraction_per_trade > 1.0) {
        throw std::invalid_argument("risk fraction must be in (0, 1]");
    }
    if (!std::isfinite(risk.max_gross_leverage) || risk.max_gross_leverage <= 0.0) {
        throw std::invalid_argument("maximum gross leverage must be finite and positive");
    }
    if (!finite_non_negative(risk.minimum_quantity)
        || !finite_non_negative(risk.quantity_step)) {
        throw std::invalid_argument("quantity limits must be finite and non-negative");
    }
    if (same_bar_policy != SameBarPolicy::pessimistic_stop_first) {
        throw std::invalid_argument("only pessimistic same-bar execution is supported");
    }
}

BacktestResult BacktestEngine::run(
    const std::span<const MarketBar> bars,
    const IStrategy& strategy,
    const StrategyParameters& parameters,
    const BacktestConfig& config) const {
    parameters.validate_for_execution();
    config.validate();

    for (std::size_t index = 0; index < bars.size(); ++index) {
        bars[index].validate();
        if (index > 0 && bars[index - 1].timestamp >= bars[index].timestamp) {
            throw std::invalid_argument("bar timestamps must be strictly increasing");
        }
    }

    BacktestResult result;
    result.summary.initial_equity = config.risk.initial_equity;
    result.summary.final_equity = config.risk.initial_equity;
    if (bars.empty()) {
        return result;
    }

    double equity = config.risk.initial_equity;
    double peak_equity = equity;
    double max_drawdown = 0.0;
    double max_drawdown_fraction = 0.0;
    std::optional<PendingSignal> pending;
    std::optional<OpenPosition> position;

    const auto close_position = [&](const ExitDecision& decision, const Timestamp exit_time) {
        const double exit_fee = order_fee(
            config.execution.fees,
            position->quantity,
            decision.fill_price);
        const double signed_move = position->side == PositionSide::long_position
            ? decision.fill_price - position->entry_price
            : position->entry_price - decision.fill_price;
        const double gross_pnl = signed_move * position->quantity;
        const double total_fees = position->entry_fee + exit_fee;
        const double net_pnl = gross_pnl - total_fees;
        equity += net_pnl;

        peak_equity = std::max(peak_equity, equity);
        const double drawdown = peak_equity - equity;
        max_drawdown = std::max(max_drawdown, drawdown);
        if (peak_equity > 0.0) {
            max_drawdown_fraction = std::max(
                max_drawdown_fraction,
                drawdown / peak_equity);
        }

        result.trades.push_back(TradeRecord{
            position->signal_timestamp,
            position->entry_timestamp,
            exit_time,
            position->side,
            decision.reason,
            decision.same_bar_ambiguity,
            position->risk_budget,
            position->quantity,
            position->entry_price,
            decision.fill_price,
            position->stop_loss,
            position->take_profit,
            gross_pnl,
            total_fees,
            net_pnl,
            equity,
            position->rule_id});
        position.reset();
    };

    for (std::size_t index = 0; index < bars.size(); ++index) {
        const MarketBar& bar = bars[index];
        const double spread = bar.bid_ask_spread.value_or(config.execution.default_spread);

        if (pending && !position) {
            const TradeSignal signal = pending->signal;
            const double half_spread = spread * 0.5;
            const double entry_quote = signal.side == PositionSide::long_position
                ? bar.open + half_spread
                : bar.open - half_spread;
            const double entry_price = adverse_fill(
                signal.side,
                true,
                entry_quote,
                config.execution.slippage_per_fill);

            const bool stop_is_valid = std::isfinite(signal.stop_loss)
                && (signal.side == PositionSide::long_position
                    ? signal.stop_loss < entry_price
                    : signal.stop_loss > entry_price);
            const bool target_is_valid = !signal.take_profit
                || (std::isfinite(*signal.take_profit)
                    && (signal.side == PositionSide::long_position
                        ? *signal.take_profit > entry_price
                        : *signal.take_profit < entry_price));

            if (!stop_is_valid || !target_is_valid || equity <= 0.0) {
                ++result.rejected_signals;
                pending.reset();
            } else {
                const double stop_fill = adverse_fill(
                    signal.side,
                    false,
                    signal.stop_loss,
                    config.execution.slippage_per_fill);
                double risk_per_unit = signal.side == PositionSide::long_position
                    ? entry_price - stop_fill
                    : stop_fill - entry_price;
                const double fee_rate = config.execution.fees.proportional_bps * kBpsScale;
                risk_per_unit += fee_rate * (std::abs(entry_price) + std::abs(stop_fill));

                const double risk_budget = equity * config.risk.risk_fraction_per_trade;
                const double fixed_fees = 2.0 * config.execution.fees.fixed_per_order;
                double quantity = risk_budget > fixed_fees && risk_per_unit > 0.0
                    ? (risk_budget - fixed_fees) / risk_per_unit
                    : 0.0;
                quantity = std::min(
                    quantity,
                    equity * config.risk.max_gross_leverage / std::abs(entry_price));
                if (config.risk.quantity_step > 0.0) {
                    quantity = std::floor(quantity / config.risk.quantity_step)
                        * config.risk.quantity_step;
                }

                if (!std::isfinite(quantity)
                    || quantity <= 0.0
                    || quantity < config.risk.minimum_quantity) {
                    ++result.rejected_signals;
                    pending.reset();
                } else {
                    position = OpenPosition{
                        pending->signal_timestamp,
                        bar.timestamp,
                        signal.side,
                        risk_budget,
                        quantity,
                        entry_price,
                        order_fee(config.execution.fees, quantity, entry_price),
                        signal.stop_loss,
                        signal.take_profit,
                        signal.rule_id};
                    pending.reset();
                }
            }
        }

        if (position) {
            const auto decision = protective_exit(
                *position,
                bar,
                spread,
                config.execution.slippage_per_fill);
            if (decision) {
                close_position(*decision, bar.timestamp);
            }
        }

        // Only the history available at this close is exposed to the strategy.
        // A last-bar signal is not requested because it cannot be filled.
        if (!position && !pending && index + 1 < bars.size()) {
            if (auto signal = strategy.on_bar_close(bars.first(index + 1), index, parameters)) {
                pending = PendingSignal{bar.timestamp, std::move(*signal)};
            }
        }
    }

    if (position && config.close_open_position_at_end) {
        const MarketBar& last = bars.back();
        const double spread = last.bid_ask_spread.value_or(config.execution.default_spread);
        const double half_spread = spread * 0.5;
        const double exit_quote = position->side == PositionSide::long_position
            ? last.close - half_spread
            : last.close + half_spread;
        close_position(
            ExitDecision{
                adverse_fill(
                    position->side,
                    false,
                    exit_quote,
                    config.execution.slippage_per_fill),
                ExitReason::end_of_data,
                false},
            last.timestamp);
    }

    auto& summary = result.summary;
    summary.trade_count = result.trades.size();
    summary.final_equity = equity;
    summary.max_drawdown = max_drawdown;
    summary.max_drawdown_fraction = max_drawdown_fraction;
    for (const auto& trade : result.trades) {
        summary.gross_pnl += trade.gross_pnl;
        summary.total_fees += trade.fees;
        summary.net_pnl += trade.net_pnl;
        if (trade.net_pnl > 0.0) {
            ++summary.winning_trades;
            summary.gross_profit += trade.net_pnl;
        } else if (trade.net_pnl < 0.0) {
            ++summary.losing_trades;
            summary.gross_loss += -trade.net_pnl;
        }
    }
    summary.return_fraction = summary.net_pnl / summary.initial_equity;
    summary.win_rate = summary.trade_count == 0
        ? 0.0
        : static_cast<double>(summary.winning_trades) / static_cast<double>(summary.trade_count);
    summary.profit_factor = summary.gross_loss > 0.0
        ? summary.gross_profit / summary.gross_loss
        : (summary.gross_profit > 0.0 ? std::numeric_limits<double>::infinity() : 0.0);
    return result;
}

std::string_view to_string(const PositionSide side) noexcept {
    return side == PositionSide::long_position ? "long" : "short";
}

std::string_view to_string(const ExitReason reason) noexcept {
    switch (reason) {
    case ExitReason::stop_loss:
        return "stop_loss";
    case ExitReason::take_profit:
        return "take_profit";
    case ExitReason::end_of_data:
        return "end_of_data";
    }
    return "unknown";
}

}  // namespace spartak
