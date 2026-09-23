#include "spartak/data_v2.hpp"

#include <array>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using ByteVector = std::vector<std::byte>;

void check(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void put_u32(ByteVector& output, const std::uint32_t value) {
    for (unsigned int i = 0; i < 4; ++i) {
        output.push_back(static_cast<std::byte>((value >> (8U * i)) & 0xFFU));
    }
}

void put_u64(ByteVector& output, const std::uint64_t value) {
    for (unsigned int i = 0; i < 8; ++i) {
        output.push_back(static_cast<std::byte>((value >> (8U * i)) & 0xFFU));
    }
}

void put_i32(ByteVector& output, const std::int32_t value) {
    put_u32(output, std::bit_cast<std::uint32_t>(value));
}

void put_i64(ByteVector& output, const std::int64_t value) {
    put_u64(output, std::bit_cast<std::uint64_t>(value));
}

void put_f64(ByteVector& output, const double value) {
    put_u64(output, std::bit_cast<std::uint64_t>(value));
}

ByteVector make_file(const std::int32_t record_size = 60) {
    const std::string symbol = "EURUSD";
    const std::array<std::int64_t, 3> times{0, 300, 600};
    ByteVector bytes;
    for (const char c : std::string("XFBAR001")) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
    }
    put_i32(bytes, 1);
    put_i32(bytes, record_size);
    put_i32(bytes, 300);
    put_i32(bytes, 5);
    put_f64(bytes, 0.00001);
    put_i64(bytes, static_cast<std::int64_t>(times.size()));
    put_i64(bytes, times.front());
    put_i64(bytes, times.back());
    put_i32(bytes, static_cast<std::int32_t>(symbol.size()));
    for (const char c : symbol) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
    }
    for (std::size_t i = 0; i < times.size(); ++i) {
        const double open = 1.1000 + static_cast<double>(i) * 0.0010;
        const double close = open + 0.0005;
        put_i64(bytes, times[i]);
        put_f64(bytes, open);
        put_f64(bytes, close + 0.0002);
        put_f64(bytes, open - 0.0002);
        put_f64(bytes, close);
        put_i64(bytes, 100 + static_cast<std::int64_t>(i));
        put_i32(bytes, 12);
        put_i64(bytes, 0);
    }
    return bytes;
}

void write_bytes(const std::filesystem::path& path, const ByteVector& bytes) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("cannot create synthetic test file");
    }
    output.write(reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    if (!output) {
        throw std::runtime_error("cannot write synthetic test file");
    }
}

template <typename Function>
void expect_throw(Function&& function, const char* message) {
    try {
        function();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error(message);
}

spartak::data::Bar bar_at(const std::int64_t time, const double price) {
    return {time, price, price + 0.2, price - 0.2, price + 0.1, 10, 2, 0};
}

}  // namespace

int main() {
    try {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto root = std::filesystem::temp_directory_path()
            / ("spartak_data_v2_" + std::to_string(nonce));
        std::filesystem::create_directories(root / "EURUSD");
        const auto valid_path = root / "EURUSD" / "EURUSD_M5.bin";
        write_bytes(valid_path, make_file());

        const auto loaded = spartak::data::load_xfbar001(
            valid_path, {.require_m5 = true, .expected_symbol = std::string("EURUSD")});
        check(loaded.header.version == 1, "version mismatch");
        check(loaded.header.period_seconds == 300, "period mismatch");
        check(loaded.header.bar_count == 3, "bar count mismatch");
        check(loaded.header.first_time == 0 && loaded.header.last_time == 600,
            "endpoint mismatch");
        check(loaded.bars.size() == 3 && loaded.bars[1].spread_points == 12,
            "record layout mismatch");

        const auto recovery_path = root / "EURUSD" / "RECOVERY_M5.bin";
        write_bytes(recovery_path, make_file(1));
        const auto recovered = spartak::data::load_xfbar001(recovery_path);
        check(recovered.header.effective_record_size == 60 && recovered.warnings.size() == 1,
            "record_size=1 recovery failed");

        auto trailing = make_file();
        trailing.push_back(std::byte{0});
        const auto trailing_path = root / "EURUSD" / "TRAILING_M5.bin";
        write_bytes(trailing_path, trailing);
        expect_throw([&] { (void)spartak::data::load_xfbar001(trailing_path); },
            "trailing byte was accepted");

        std::ofstream(root / "EURUSD" / "EURUSD_M5_IND.bin").put('\0');
        std::ofstream(root / "EURUSD" / "EURUSD_H1.bin").put('\0');
        const auto discovered = spartak::data::discover_m5_files(root);
        check(discovered.size() == 3, "M5 discovery did not apply filename rules");

        const std::array<spartak::data::Bar, 4> source{
            bar_at(0, 10.0), bar_at(300, 10.1), bar_at(600, 10.2), bar_at(1200, 10.3)};
        const auto m15 = spartak::data::resample_calendar(
            source, spartak::data::Timeframe::m15);
        check(m15.size() == 2, "resampler created synthetic buckets");
        check(!m15[0].incomplete_bucket && m15[0].source_bar_count == 3,
            "complete M15 bucket misclassified");
        check(m15[1].incomplete_bucket && m15[1].gap_before
                && m15[1].source_bar_count == 1,
            "gap or partial M15 bucket was not flagged");

        std::filesystem::remove_all(root);
        std::cout << "data_v2 tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "data_v2 test failure: " << error.what() << '\n';
        return 1;
    }
}
