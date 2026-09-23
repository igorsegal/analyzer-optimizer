#include "spartak/data_v2.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

double percentile(std::vector<std::int32_t> values, const double probability) {
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const double index = probability * static_cast<double>(values.size() - 1);
    const auto lower = static_cast<std::size_t>(index);
    const auto upper = std::min(lower + 1, values.size() - 1);
    const double weight = index - static_cast<double>(lower);
    return static_cast<double>(values[lower]) * (1.0 - weight)
        + static_cast<double>(values[upper]) * weight;
}

void print_csv_row(const std::filesystem::path& path) {
    const std::string expected = path.parent_path().filename().string();
    const auto loaded = spartak::data::load_xfbar001(
        path, {.require_m5 = true, .expected_symbol = expected});
    std::uint64_t gaps{};
    std::vector<std::int32_t> spreads;
    spreads.reserve(loaded.bars.size());
    for (std::size_t i = 0; i < loaded.bars.size(); ++i) {
        spreads.push_back(loaded.bars[i].spread_points);
        if (i > 0 && loaded.bars[i].time != loaded.bars[i - 1].time + 300) {
            ++gaps;
        }
    }
    const auto m15 = spartak::data::resample_calendar(loaded.bars, spartak::data::Timeframe::m15);
    const auto h1 = spartak::data::resample_calendar(loaded.bars, spartak::data::Timeframe::h1);
    const auto count_incomplete = [](const auto& bars) {
        return static_cast<std::uint64_t>(std::count_if(
            bars.begin(), bars.end(), [](const auto& bar) { return bar.incomplete_bucket; }));
    };
    std::cout << loaded.header.symbol << ','
              << loaded.header.bar_count << ','
              << loaded.header.first_time << ','
              << loaded.header.last_time << ','
              << loaded.header.digits << ','
              << std::setprecision(12) << loaded.header.point << ','
              << gaps << ','
              << std::fixed << std::setprecision(2)
              << percentile(spreads, 0.50) << ','
              << percentile(spreads, 0.95) << ','
              << percentile(spreads, 0.99) << ','
              << m15.size() << ',' << count_incomplete(m15) << ','
              << h1.size() << ',' << count_incomplete(h1) << ','
              << loaded.warnings.size() << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: spartak_inspect_v2 <XFBAR001 file or raw root>\n";
            return 2;
        }
        const std::filesystem::path input = argv[1];
        std::cout << "symbol,bars,first_time,last_time,digits,point,gaps,spread_p50_points,"
                     "spread_p95_points,spread_p99_points,m15_bars,m15_incomplete,h1_bars,"
                     "h1_incomplete,warnings\n";
        if (std::filesystem::is_directory(input)) {
            const auto files = spartak::data::discover_m5_files(input);
            for (const auto& path : files) {
                print_csv_row(path);
            }
            return files.empty() ? 3 : 0;
        }
        print_csv_row(input);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "inspect error: " << error.what() << '\n';
        return 1;
    }
}
