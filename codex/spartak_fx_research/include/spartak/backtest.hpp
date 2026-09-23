#pragma once

#include "spartak/strategy.hpp"

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace spartak {

struct FeeSchedule {
    double fixed_per_order{};
    double proportional_bps{};
};

struct ExecutionConfig {
    double default_spread{};
    double slippage_per_fill{};
    FeeSchedule fees;
};

struct RiskConfig {
    double initial_equity{};
    double risk_fraction_per_trade{};
    double max_gross_leverage{};
    double minimum_quantity{};
    double quantity_step{};
};

enum class SameBarPolicy {
    pessimistic_stop_first,
};

struct BacktestConfig {
    ExecutionConfig execution;
    RiskConfig risk;
    SameBarPolicy same_bar_policy{SameBarPolicy::pessimistic_stop_first};
    bool close_open_position_at_end{true};

    void validate() const;
};

enum class ExitReason {
    stop_loss,
    take_profit,
    end_of_data,
};

struct TradeRecord {
    Timestamp signal_timestamp{};
    Timestamp entry_timestamp{};
    Timestamp exit_timestamp{};
    PositionSide side{PositionSide::long_position};
    ExitReason exit_reason{ExitReason::end_of_data};
    bool same_bar_ambiguity{};
    double risk_budget{};
    double quantity{};
    double entry_price{};
    double exit_price{};
    double stop_loss{};
    std::optional<double> take_profit;
    double gross_pnl{};
    double fees{};
    double net_pnl{};
    double equity_after{};
    std::string rule_id;
};

struct PerformanceSummary {
    std::size_t trade_count{};
    std::size_t winning_trades{};
    std::size_t losing_trades{};
    double initial_equity{};
    double final_equity{};
    double gross_pnl{};
    double total_fees{};
    double net_pnl{};
    double return_fraction{};
    double gross_profit{};
    double gross_loss{};
    double profit_factor{};
    double win_rate{};
    double max_drawdown{};
    double max_drawdown_fraction{};
};

struct BacktestResult {
    std::vector<TradeRecord> trades;
    PerformanceSummary summary;
    std::size_t rejected_signals{};
};

class BacktestEngine {
public:
    [[nodiscard]] BacktestResult run(
        std::span<const MarketBar> bars,
        const IStrategy& strategy,
        const StrategyParameters& parameters,
        const BacktestConfig& config) const;
};

[[nodiscard]] std::string_view to_string(PositionSide side) noexcept;
[[nodiscard]] std::string_view to_string(ExitReason reason) noexcept;

}  // namespace spartak
