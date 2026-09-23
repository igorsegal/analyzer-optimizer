#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace spartak::specs {

enum class AssetClass {
    spot_fx,
    metal,
    crypto,
};

struct InstrumentSpec {
    std::string symbol;
    AssetClass asset_class{AssetClass::spot_fx};
    double minimum_lot{};
    double lot_step{};
    double tick_value{};
    double tick_size{};
    std::string profit_currency;
    double leverage{};
    std::string source_snapshot;
    std::optional<double> commission_per_lot_per_side;
    std::optional<double> swap_or_funding;
};

class SpecBook {
public:
    explicit SpecBook(std::vector<InstrumentSpec> rows);

    [[nodiscard]] const std::vector<InstrumentSpec>& rows() const noexcept { return rows_; }
    [[nodiscard]] const InstrumentSpec& require(std::string_view symbol) const;

private:
    std::vector<InstrumentSpec> rows_;
};

[[nodiscard]] SpecBook load_normalized_csv(const std::filesystem::path& path);
[[nodiscard]] std::string_view to_string(AssetClass value) noexcept;

}  // namespace spartak::specs
