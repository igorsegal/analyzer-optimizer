#include "spartak/specs.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace spartak::specs {
namespace {

[[nodiscard]] std::vector<std::string> parse_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;
    bool quote_closed = false;

    for (std::size_t index = 0; index < line.size(); ++index) {
        const char character = line[index];
        if (quoted) {
            if (character == '"') {
                if (index + 1 < line.size() && line[index + 1] == '"') {
                    field.push_back('"');
                    ++index;
                } else {
                    quoted = false;
                    quote_closed = true;
                }
            } else {
                field.push_back(character);
            }
            continue;
        }

        if (character == ',') {
            fields.push_back(std::move(field));
            field.clear();
            quote_closed = false;
        } else if (character == '"') {
            if (!field.empty() || quote_closed) {
                throw std::invalid_argument("malformed quote in CSV field");
            }
            quoted = true;
        } else {
            if (quote_closed) {
                throw std::invalid_argument("characters follow a closed quoted CSV field");
            }
            field.push_back(character);
        }
    }
    if (quoted) {
        throw std::invalid_argument("unterminated quoted CSV field");
    }
    fields.push_back(std::move(field));
    return fields;
}

[[nodiscard]] double parse_positive(
    const std::string& text,
    const std::string_view field_name,
    const bool allow_zero = false) {
    double value{};
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto parsed = std::from_chars(begin, end, value, std::chars_format::general);
    if (parsed.ec != std::errc{} || parsed.ptr != end || !std::isfinite(value)
        || (allow_zero ? value < 0.0 : value <= 0.0)) {
        throw std::invalid_argument(
            std::string{field_name} + " must be a dot-decimal finite positive number");
    }
    return value;
}

[[nodiscard]] std::optional<double> parse_optional_cost(
    const std::string& text,
    const std::string_view field_name) {
    if (text == "REQUIRED_EXTERNAL") {
        return std::nullopt;
    }
    return parse_positive(text, field_name, true);
}

[[nodiscard]] AssetClass parse_asset_class(const std::string& text) {
    if (text == "spot_fx") {
        return AssetClass::spot_fx;
    }
    if (text == "metal") {
        return AssetClass::metal;
    }
    if (text == "crypto") {
        return AssetClass::crypto;
    }
    throw std::invalid_argument("unknown asset_class: " + text);
}

[[nodiscard]] std::map<std::string, std::size_t, std::less<>> header_map(
    const std::vector<std::string>& header) {
    std::map<std::string, std::size_t, std::less<>> result;
    for (std::size_t index = 0; index < header.size(); ++index) {
        if (header[index].empty() || !result.emplace(header[index], index).second) {
            throw std::invalid_argument("CSV header names must be non-empty and unique");
        }
    }
    constexpr std::string_view required[]{
        "symbol",
        "asset_class",
        "min_lot",
        "step_lot",
        "tick_value",
        "tick_size",
        "currency_profit",
        "leverage",
        "source_snapshot",
        "commission",
        "swap_or_funding",
    };
    for (const auto name : required) {
        if (!result.contains(name)) {
            throw std::invalid_argument("missing required CSV column: " + std::string{name});
        }
    }
    return result;
}

[[nodiscard]] const std::string& field(
    const std::vector<std::string>& row,
    const std::map<std::string, std::size_t, std::less<>>& columns,
    const std::string_view name) {
    const std::size_t index = columns.at(std::string{name});
    if (index >= row.size()) {
        throw std::invalid_argument("CSV row is shorter than its header");
    }
    return row[index];
}

}  // namespace

SpecBook::SpecBook(std::vector<InstrumentSpec> rows) : rows_(std::move(rows)) {
    if (rows_.empty()) {
        throw std::invalid_argument("spec book must not be empty");
    }
    std::set<std::string, std::less<>> symbols;
    for (const auto& row : rows_) {
        if (row.symbol.empty() || !symbols.insert(row.symbol).second) {
            throw std::invalid_argument("spec symbols must be non-empty and unique");
        }
    }
}

const InstrumentSpec& SpecBook::require(const std::string_view symbol) const {
    const auto found = std::find_if(
        rows_.begin(),
        rows_.end(),
        [symbol](const InstrumentSpec& row) { return row.symbol == symbol; });
    if (found == rows_.end()) {
        throw std::out_of_range("missing instrument spec: " + std::string{symbol});
    }
    return *found;
}

SpecBook load_normalized_csv(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    if (!input) {
        throw std::runtime_error("cannot open normalized specs CSV: " + path.string());
    }

    std::string line;
    if (!std::getline(input, line)) {
        throw std::invalid_argument("normalized specs CSV is empty");
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    if (line.size() >= 3
        && static_cast<unsigned char>(line[0]) == 0xEF
        && static_cast<unsigned char>(line[1]) == 0xBB
        && static_cast<unsigned char>(line[2]) == 0xBF) {
        line.erase(0, 3);
    }
    const auto header = parse_csv_line(line);
    const auto columns = header_map(header);

    std::vector<InstrumentSpec> rows;
    std::set<std::string, std::less<>> symbols;
    std::size_t line_number = 1;
    while (std::getline(input, line)) {
        ++line_number;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        const auto values = parse_csv_line(line);
        if (values.size() != header.size()) {
            throw std::invalid_argument(
                "CSV field count differs from header at line " + std::to_string(line_number));
        }
        const std::string symbol = field(values, columns, "symbol");
        if (symbol.empty() || !symbols.insert(symbol).second) {
            throw std::invalid_argument(
                "duplicate or empty symbol at line " + std::to_string(line_number));
        }
        const std::string profit_currency = field(values, columns, "currency_profit");
        const std::string snapshot = field(values, columns, "source_snapshot");
        if (profit_currency.empty() || snapshot.empty()) {
            throw std::invalid_argument("profit currency and snapshot must be present");
        }
        rows.push_back(InstrumentSpec{
            symbol,
            parse_asset_class(field(values, columns, "asset_class")),
            parse_positive(field(values, columns, "min_lot"), "min_lot"),
            parse_positive(field(values, columns, "step_lot"), "step_lot"),
            parse_positive(field(values, columns, "tick_value"), "tick_value"),
            parse_positive(field(values, columns, "tick_size"), "tick_size"),
            profit_currency,
            parse_positive(field(values, columns, "leverage"), "leverage"),
            snapshot,
            parse_optional_cost(field(values, columns, "commission"), "commission"),
            parse_optional_cost(field(values, columns, "swap_or_funding"), "swap_or_funding"),
        });
    }
    return SpecBook{std::move(rows)};
}

std::string_view to_string(const AssetClass value) noexcept {
    switch (value) {
    case AssetClass::spot_fx:
        return "spot_fx";
    case AssetClass::metal:
        return "metal";
    case AssetClass::crypto:
        return "crypto";
    }
    return "unknown";
}

}  // namespace spartak::specs
