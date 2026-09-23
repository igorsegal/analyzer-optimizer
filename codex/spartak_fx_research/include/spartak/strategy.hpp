#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace spartak {

using Timestamp = std::int64_t;

// Mid-price OHLC bar. bid_ask_spread may override the experiment-wide spread
// for this bar. Parser/resampler code can adapt its own bar type to MarketBar.
struct MarketBar {
    Timestamp timestamp{};
    double open{};
    double high{};
    double low{};
    double close{};
    std::optional<double> bid_ask_spread;

    void validate() const;
};

enum class PositionSide {
    long_position,
    short_position,
};

struct SourceReference {
    std::string asset;
    std::string timestamp;
    std::string note;
};

// The strategy definition deliberately remains generic until the video rules
// have been transcribed and reviewed. Execution is blocked while source_pending
// is true, so placeholder values cannot accidentally become research results.
struct StrategyParameters {
    bool source_pending{true};
    std::string rule_revision{"source_pending"};
    std::map<std::string, double, std::less<>> numeric;
    std::map<std::string, bool, std::less<>> boolean;
    std::vector<SourceReference> sources;

    void validate_for_execution() const;
    [[nodiscard]] double require_numeric(std::string_view name) const;
    [[nodiscard]] bool require_boolean(std::string_view name) const;
};

// stop_loss and take_profit are executable exit-quote price levels: bid for a
// long position, ask for a short position. They are created at signal close and
// validated again against the actual next-bar entry fill.
struct TradeSignal {
    PositionSide side{PositionSide::long_position};
    double stop_loss{};
    std::optional<double> take_profit;
    std::string rule_id;
};

class IStrategy {
public:
    virtual ~IStrategy() = default;

    [[nodiscard]] virtual std::optional<TradeSignal> on_bar_close(
        std::span<const MarketBar> bars,
        std::size_t bar_index,
        const StrategyParameters& parameters) const = 0;
};

// Explicit non-strategy used until the source-derived implementation exists.
// It throws rather than silently returning an empty signal stream.
class SourcePendingStrategy final : public IStrategy {
public:
    [[nodiscard]] std::optional<TradeSignal> on_bar_close(
        std::span<const MarketBar> bars,
        std::size_t bar_index,
        const StrategyParameters& parameters) const override;
};

[[nodiscard]] std::string canonical_parameter_key(const StrategyParameters& parameters);

}  // namespace spartak
