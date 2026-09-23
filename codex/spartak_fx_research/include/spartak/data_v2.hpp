#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace spartak::data {

inline constexpr std::int32_t kXfbarVersion = 1;
inline constexpr std::int32_t kXfbarRecordBytes = 60;
inline constexpr std::int32_t kM5Seconds = 300;

struct Bar {
    std::int64_t time{};  // Unix seconds: opening time of the bar.
    double open{};
    double high{};
    double low{};
    double close{};
    std::int64_t tick_volume{};
    std::int32_t spread_points{};
    std::int64_t real_volume{};
};

struct Header {
    std::int32_t version{};
    std::int32_t declared_record_size{};
    std::int32_t effective_record_size{};
    std::int32_t period_seconds{};
    std::int32_t digits{};
    double point{};
    std::int64_t bar_count{};
    std::int64_t first_time{};
    std::int64_t last_time{};
    std::string symbol;
};

struct Warning {
    std::string code;
    std::string message;
};

struct ParseOptions {
    bool allow_record_size_one_recovery{true};
    bool require_m5{false};
    std::optional<std::string> expected_symbol;
};

struct LoadedFile {
    std::filesystem::path path;
    Header header;
    std::vector<Bar> bars;
    std::vector<Warning> warnings;
};

[[nodiscard]] LoadedFile load_xfbar001(
    const std::filesystem::path& path,
    const ParseOptions& options = {});

[[nodiscard]] std::vector<std::filesystem::path> discover_m5_files(
    const std::filesystem::path& root);

enum class Timeframe : std::int32_t {
    m15 = 900,
    h1 = 3600,
    d1 = 86400,
};

struct ResampleOptions {
    // Session boundary relative to UTC. Used only for D1. Must be a multiple
    // of 300 seconds and strictly between -86400 and +86400.
    std::int32_t daily_offset_seconds{};
};

struct ResampledBar {
    Bar bar;
    std::uint32_t source_bar_count{};
    bool incomplete_bucket{};
    bool gap_before{};
};

[[nodiscard]] std::vector<ResampledBar> resample_calendar(
    std::span<const Bar> m5,
    Timeframe target,
    const ResampleOptions& options = {});

}  // namespace spartak::data
