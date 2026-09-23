#include "spartak/data_v2.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>

namespace spartak::data {
namespace {

using Bytes60 = std::array<std::byte, 60>;
constexpr std::array<char, 8> kMagic{'X', 'F', 'B', 'A', 'R', '0', '0', '1'};

[[noreturn]] void fail(const std::filesystem::path& path, const std::string& message) {
    throw std::runtime_error(path.string() + ": " + message);
}

void read_exact(
    std::ifstream& input,
    std::byte* destination,
    const std::size_t size,
    const std::filesystem::path& path,
    const char* label) {
    if (size > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
        fail(path, std::string(label) + " is too large for stream I/O");
    }
    input.read(reinterpret_cast<char*>(destination), static_cast<std::streamsize>(size));
    if (!input || input.gcount() != static_cast<std::streamsize>(size)) {
        fail(path, std::string("truncated ") + label);
    }
}

[[nodiscard]] std::uint32_t read_u32(const Bytes60& bytes, const std::size_t offset) {
    std::uint32_t value{};
    for (std::size_t i = 0; i < 4; ++i) {
        value |= static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + i]))
            << static_cast<unsigned int>(8 * i);
    }
    return value;
}

[[nodiscard]] std::uint64_t read_u64(const Bytes60& bytes, const std::size_t offset) {
    std::uint64_t value{};
    for (std::size_t i = 0; i < 8; ++i) {
        value |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(bytes[offset + i]))
            << static_cast<unsigned int>(8 * i);
    }
    return value;
}

[[nodiscard]] std::int32_t read_i32(const Bytes60& bytes, const std::size_t offset) {
    return std::bit_cast<std::int32_t>(read_u32(bytes, offset));
}

[[nodiscard]] std::int64_t read_i64(const Bytes60& bytes, const std::size_t offset) {
    return std::bit_cast<std::int64_t>(read_u64(bytes, offset));
}

[[nodiscard]] double read_f64(const Bytes60& bytes, const std::size_t offset) {
    return std::bit_cast<double>(read_u64(bytes, offset));
}

[[nodiscard]] bool valid_utf8(const std::vector<std::byte>& bytes) {
    std::size_t i{};
    while (i < bytes.size()) {
        const auto first = std::to_integer<unsigned char>(bytes[i]);
        if (first == 0) {
            return false;
        }
        if (first < 0x80U) {
            ++i;
            continue;
        }
        std::size_t continuation_count{};
        std::uint32_t code_point{};
        std::uint32_t minimum{};
        if ((first & 0xE0U) == 0xC0U) {
            continuation_count = 1;
            code_point = first & 0x1FU;
            minimum = 0x80U;
        } else if ((first & 0xF0U) == 0xE0U) {
            continuation_count = 2;
            code_point = first & 0x0FU;
            minimum = 0x800U;
        } else if ((first & 0xF8U) == 0xF0U) {
            continuation_count = 3;
            code_point = first & 0x07U;
            minimum = 0x10000U;
        } else {
            return false;
        }
        if (i + continuation_count >= bytes.size()) {
            return false;
        }
        for (std::size_t j = 1; j <= continuation_count; ++j) {
            const auto next = std::to_integer<unsigned char>(bytes[i + j]);
            if ((next & 0xC0U) != 0x80U) {
                return false;
            }
            code_point = (code_point << 6U) | (next & 0x3FU);
        }
        if (code_point < minimum || code_point > 0x10FFFFU
            || (code_point >= 0xD800U && code_point <= 0xDFFFU)) {
            return false;
        }
        i += continuation_count + 1;
    }
    return true;
}

[[nodiscard]] bool checked_mul_add_size(
    const std::uint64_t count,
    const std::uint64_t record_size,
    const std::uint64_t prefix,
    std::uint64_t& result) {
    if (count != 0 && record_size > std::numeric_limits<std::uint64_t>::max() / count) {
        return false;
    }
    const std::uint64_t payload = count * record_size;
    if (prefix > std::numeric_limits<std::uint64_t>::max() - payload) {
        return false;
    }
    result = prefix + payload;
    return true;
}

void validate_bar(const Bar& bar, const std::filesystem::path* path = nullptr) {
    const auto reject = [&](const char* message) {
        if (path != nullptr) {
            fail(*path, message);
        }
        throw std::invalid_argument(message);
    };
    if (!std::isfinite(bar.open) || !std::isfinite(bar.high)
        || !std::isfinite(bar.low) || !std::isfinite(bar.close)) {
        reject("OHLC contains a non-finite value");
    }
    if (bar.open <= 0.0 || bar.high <= 0.0 || bar.low <= 0.0 || bar.close <= 0.0) {
        reject("OHLC prices must be positive");
    }
    if (bar.high < std::max(bar.open, bar.close)
        || bar.low > std::min(bar.open, bar.close)
        || bar.high < bar.low) {
        reject("OHLC ordering is invalid");
    }
    if (bar.tick_volume < 0 || bar.spread_points < 0 || bar.real_volume < 0) {
        reject("volumes and spread must be non-negative");
    }
}

[[nodiscard]] std::string ascii_lower(std::string value) {
    for (char& c : value) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return value;
}

[[nodiscard]] std::int64_t floor_bucket(
    const std::int64_t time,
    const std::int64_t interval,
    const std::int64_t offset) {
    if ((offset > 0 && time < std::numeric_limits<std::int64_t>::min() + offset)
        || (offset < 0 && time > std::numeric_limits<std::int64_t>::max() + offset)) {
        throw std::overflow_error("timestamp offset overflow");
    }
    const std::int64_t shifted = time - offset;
    std::int64_t quotient = shifted / interval;
    if (shifted % interval < 0) {
        --quotient;
    }
    if (quotient > std::numeric_limits<std::int64_t>::max() / interval
        || quotient < std::numeric_limits<std::int64_t>::min() / interval) {
        throw std::overflow_error("timestamp bucket overflow");
    }
    const std::int64_t base = quotient * interval;
    if ((offset > 0 && base > std::numeric_limits<std::int64_t>::max() - offset)
        || (offset < 0 && base < std::numeric_limits<std::int64_t>::min() - offset)) {
        throw std::overflow_error("timestamp bucket offset overflow");
    }
    return base + offset;
}

void add_nonnegative(std::int64_t& destination, const std::int64_t value) {
    if (value < 0 || destination > std::numeric_limits<std::int64_t>::max() - value) {
        throw std::overflow_error("aggregated volume overflow");
    }
    destination += value;
}

}  // namespace

LoadedFile load_xfbar001(const std::filesystem::path& path, const ParseOptions& options) {
    std::error_code error;
    const std::uintmax_t platform_size = std::filesystem::file_size(path, error);
    if (error) {
        fail(path, "cannot determine file size");
    }
    if (platform_size < 60U || platform_size > std::numeric_limits<std::uint64_t>::max()) {
        fail(path, "file size is outside supported bounds");
    }
    const auto file_size = static_cast<std::uint64_t>(platform_size);

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        fail(path, "cannot open input");
    }
    Bytes60 fixed{};
    read_exact(input, fixed.data(), fixed.size(), path, "fixed header");
    for (std::size_t i = 0; i < kMagic.size(); ++i) {
        if (std::to_integer<unsigned char>(fixed[i]) != static_cast<unsigned char>(kMagic[i])) {
            fail(path, "magic is not XFBAR001");
        }
    }

    Header header;
    header.version = read_i32(fixed, 8);
    header.declared_record_size = read_i32(fixed, 12);
    header.period_seconds = read_i32(fixed, 16);
    header.digits = read_i32(fixed, 20);
    header.point = read_f64(fixed, 24);
    header.bar_count = read_i64(fixed, 32);
    header.first_time = read_i64(fixed, 40);
    header.last_time = read_i64(fixed, 48);
    const std::int32_t symbol_length = read_i32(fixed, 56);

    if (header.version != kXfbarVersion) {
        fail(path, "unsupported version");
    }
    if (header.period_seconds <= 0 || (options.require_m5 && header.period_seconds != kM5Seconds)) {
        fail(path, "invalid or unexpected period_seconds");
    }
    if (header.digits < 0 || header.digits > 15 || !std::isfinite(header.point) || header.point <= 0.0) {
        fail(path, "invalid digits or point");
    }
    if (header.bar_count < 0 || symbol_length <= 0 || symbol_length > 4096) {
        fail(path, "invalid bar_count or symbol_len");
    }

    const std::uint64_t data_offset = 60U + static_cast<std::uint64_t>(symbol_length);
    if (data_offset > file_size) {
        fail(path, "symbol extends beyond file bounds");
    }
    const auto count = static_cast<std::uint64_t>(header.bar_count);
    const std::uint64_t payload_size = file_size - data_offset;
    LoadedFile loaded;
    loaded.path = path;
    if (header.declared_record_size == kXfbarRecordBytes) {
        header.effective_record_size = kXfbarRecordBytes;
    } else if (header.declared_record_size == 1 && options.allow_record_size_one_recovery
        && count <= std::numeric_limits<std::uint64_t>::max() / 60U
        && payload_size == count * 60U) {
        header.effective_record_size = kXfbarRecordBytes;
        loaded.warnings.push_back({
            "record_size_recovered",
            "declared record_size=1 recovered as 60 after exact payload validation"});
    } else {
        fail(path, "record_size is not 60 and cannot be safely recovered");
    }
    std::uint64_t expected_size{};
    if (!checked_mul_add_size(count, 60U, data_offset, expected_size) || expected_size != file_size) {
        fail(path, "file size does not equal 60 + symbol_len + bar_count*60");
    }

    std::vector<std::byte> symbol_bytes(static_cast<std::size_t>(symbol_length));
    read_exact(input, symbol_bytes.data(), symbol_bytes.size(), path, "UTF-8 symbol");
    if (!valid_utf8(symbol_bytes)) {
        fail(path, "symbol is not valid non-NUL UTF-8");
    }
    header.symbol.assign(
        reinterpret_cast<const char*>(symbol_bytes.data()), symbol_bytes.size());
    if (options.expected_symbol.has_value() && header.symbol != *options.expected_symbol) {
        fail(path, "header symbol does not match expected symbol");
    }
    loaded.header = header;
    if (count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        fail(path, "bar count does not fit in memory");
    }
    loaded.bars.reserve(static_cast<std::size_t>(count));
    Bytes60 record{};
    for (std::uint64_t i = 0; i < count; ++i) {
        read_exact(input, record.data(), record.size(), path, "bar record");
        Bar bar{
            read_i64(record, 0),
            read_f64(record, 8),
            read_f64(record, 16),
            read_f64(record, 24),
            read_f64(record, 32),
            read_i64(record, 40),
            read_i32(record, 48),
            read_i64(record, 52)};
        validate_bar(bar, &path);
        if (!loaded.bars.empty() && loaded.bars.back().time >= bar.time) {
            fail(path, "timestamps are not strictly ascending");
        }
        loaded.bars.push_back(bar);
    }
    if (loaded.bars.empty()) {
        if (header.first_time != 0 || header.last_time != 0) {
            fail(path, "empty series has non-zero endpoints");
        }
    } else if (loaded.bars.front().time != header.first_time
        || loaded.bars.back().time != header.last_time) {
        fail(path, "header endpoints do not match records");
    }
    return loaded;
}

std::vector<std::filesystem::path> discover_m5_files(const std::filesystem::path& root) {
    std::vector<std::filesystem::path> result;
    std::error_code error;
    const std::filesystem::recursive_directory_iterator end;
    for (std::filesystem::recursive_directory_iterator it(
             root, std::filesystem::directory_options::skip_permission_denied, error);
         it != end;
         it.increment(error)) {
        if (error) {
            error.clear();
            continue;
        }
        if (!it->is_regular_file(error) || error) {
            error.clear();
            continue;
        }
        const std::string name = ascii_lower(it->path().filename().string());
        if (name.size() >= 7 && name.ends_with("_m5.bin")
            && name.find("_ind") == std::string::npos) {
            result.push_back(it->path());
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<ResampledBar> resample_calendar(
    const std::span<const Bar> m5,
    const Timeframe target,
    const ResampleOptions& options) {
    const std::int64_t interval = static_cast<std::int32_t>(target);
    if (interval != 900 && interval != 3600 && interval != 86400) {
        throw std::invalid_argument("unsupported target timeframe");
    }
    std::int64_t offset{};
    if (target == Timeframe::d1) {
        offset = options.daily_offset_seconds;
        if (offset <= -86400 || offset >= 86400 || offset % kM5Seconds != 0) {
            throw std::invalid_argument("daily offset must be within one day and M5-aligned");
        }
    }
    const auto expected = static_cast<std::uint32_t>(interval / kM5Seconds);
    for (std::size_t i = 0; i < m5.size(); ++i) {
        validate_bar(m5[i]);
        if (m5[i].time % kM5Seconds != 0) {
            throw std::invalid_argument("M5 timestamp is not 300-second aligned");
        }
        if (i > 0 && m5[i - 1].time >= m5[i].time) {
            throw std::invalid_argument("M5 timestamps are not strictly ascending");
        }
    }
    std::vector<ResampledBar> output;
    if (m5.empty()) {
        return output;
    }

    ResampledBar current{};
    std::int64_t current_bucket{};
    std::int64_t previous_bucket{};
    std::int64_t previous_source{};
    bool have_current = false;
    bool sequence_complete = true;
    bool starts_at_bucket = true;

    const auto finalize = [&]() {
        const std::int64_t expected_last = current_bucket
            + (static_cast<std::int64_t>(expected) - 1) * kM5Seconds;
        current.incomplete_bucket = !sequence_complete || !starts_at_bucket
            || current.source_bar_count != expected || previous_source != expected_last;
        output.push_back(current);
    };

    for (const Bar& source : m5) {
        const std::int64_t bucket = floor_bucket(source.time, interval, offset);
        if (!have_current || bucket != current_bucket) {
            if (have_current) {
                finalize();
                previous_bucket = current_bucket;
            }
            const bool has_source_gap = have_current && source.time != previous_source + kM5Seconds;
            const bool has_bucket_gap = have_current && bucket != previous_bucket + interval;
            current_bucket = bucket;
            current = ResampledBar{
                Bar{bucket, source.open, source.high, source.low, source.close,
                    0, source.spread_points, 0},
                0,
                false,
                source.time != bucket || has_source_gap || has_bucket_gap};
            sequence_complete = true;
            starts_at_bucket = source.time == bucket;
            have_current = true;
        }
        if (current.source_bar_count >= expected) {
            throw std::invalid_argument("calendar bucket contains too many M5 bars");
        }
        const std::int64_t expected_time = current_bucket
            + static_cast<std::int64_t>(current.source_bar_count) * kM5Seconds;
        if (source.time != expected_time) {
            sequence_complete = false;
        }
        current.bar.high = std::max(current.bar.high, source.high);
        current.bar.low = std::min(current.bar.low, source.low);
        current.bar.close = source.close;
        add_nonnegative(current.bar.tick_volume, source.tick_volume);
        add_nonnegative(current.bar.real_volume, source.real_volume);
        current.bar.spread_points = std::max(current.bar.spread_points, source.spread_points);
        ++current.source_bar_count;
        previous_source = source.time;
    }
    finalize();
    return output;
}

}  // namespace spartak::data
