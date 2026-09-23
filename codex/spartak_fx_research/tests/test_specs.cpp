#include "spartak/specs.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(const bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class TemporaryCsv {
public:
    explicit TemporaryCsv(const std::string& contents) {
        static std::size_t counter = 0;
        path_ = std::filesystem::temp_directory_path()
            / ("spartak_specs_test_" + std::to_string(++counter) + ".csv");
        std::ofstream output{path_, std::ios::binary};
        output << contents;
        if (!output) {
            throw std::runtime_error("failed to write temporary specs CSV");
        }
    }

    ~TemporaryCsv() {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

constexpr const char* kHeader =
    "symbol,asset_class,min_lot,step_lot,tick_value,tick_size,currency_profit,"
    "leverage,source_snapshot,commission,swap_or_funding\n";

void test_valid_and_external_costs() {
    TemporaryCsv file{
        std::string{kHeader}
        + "GBPJPY,spot_fx,0.01,0.01,0.6197937326,0.001,JPY,100,2026-07-07,"
          "REQUIRED_EXTERNAL,REQUIRED_EXTERNAL\n"
        + "\"BTCUSD\",\"crypto\",\"1\",\"1\",\"0.001\",\"0.1\","
          "\"USD\",\"100\",\"2026-07-07\",\"2.5\",\"0\"\n"};
    const auto book = spartak::specs::load_normalized_csv(file.path());
    require(book.rows().size() == 2, "wrong spec row count");
    const auto& fx = book.require("GBPJPY");
    require(!fx.commission_per_lot_per_side, "external commission became zero");
    require(!fx.swap_or_funding, "external swap/funding became zero");
    const auto& crypto = book.require("BTCUSD");
    require(crypto.commission_per_lot_per_side
            && *crypto.commission_per_lot_per_side == 2.5,
        "numeric commission was not parsed");
    require(crypto.swap_or_funding && *crypto.swap_or_funding == 0.0,
        "explicit zero funding was not preserved");
}

void expect_failure(const std::string& contents, const std::string& message) {
    TemporaryCsv file{contents};
    bool threw = false;
    try {
        (void)spartak::specs::load_normalized_csv(file.path());
    } catch (const std::exception&) {
        threw = true;
    }
    require(threw, message);
}

void test_fail_closed_cases() {
    expect_failure(
        std::string{kHeader}
            + "EURUSD,spot_fx,0.01,0.01,1,0.00001,USD,100,2026-07-07,"
              "REQUIRED_EXTERNAL,REQUIRED_EXTERNAL\n"
            + "EURUSD,spot_fx,0.01,0.01,1,0.00001,USD,100,2026-07-07,"
              "REQUIRED_EXTERNAL,REQUIRED_EXTERNAL\n",
        "duplicate symbol was accepted");
    expect_failure(
        std::string{kHeader}
            + "EURUSD,spot_fx,0,01,0.01,1,0.00001,USD,100,2026-07-07,"
              "REQUIRED_EXTERNAL,REQUIRED_EXTERNAL\n",
        "comma decimal was accepted");
    expect_failure(
        "symbol,asset_class,min_lot\nEURUSD,spot_fx,0.01\n",
        "missing required columns were accepted");
    expect_failure(
        std::string{kHeader}
            + "EURUSD,unknown,0.01,0.01,1,0.00001,USD,100,2026-07-07,"
              "REQUIRED_EXTERNAL,REQUIRED_EXTERNAL\n",
        "unknown asset class was accepted");
}

}  // namespace

int main() {
    try {
        test_valid_and_external_costs();
        test_fail_closed_cases();
        std::cout << "instrument specs tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "instrument specs test failure: " << error.what() << '\n';
        return 1;
    }
}
