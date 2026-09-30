// Smoke-тест: DataSanitizer на реальном XFBAR-файле.
// Показывает, сколько Daily-баров в префиксе и с какого времени
// начинается регулярный M5.
#include "data/BarStream.h"
#include "data/DataSanitizer.h"
#include <iostream>
#include <iomanip>
#include <ctime>
int main(int argc, char** argv) {
    using namespace spartak;
    const std::string path = (argc > 1)
        ? argv[1]
        : "D:/AHexaTrader/1DataFiles/raw/EURUSD/EURUSD_M5.bin";
    std::cout << "=== DataSanitizer smoke test ===\n\n";
    std::cout << "Path: " << path << "\n\n";
    data::BarStream stream(path);
    if (!stream.is_ok()) {
        std::cout << "Stream open FAILED\n";
        return 1;
    }
    std::cout << "Total bars in stream : " << stream.total_bars() << "\n";
    std::cout << "Scanning for regular M5 start...\n\n";
    data::DataSanitizer sanitizer(/*max_gap=4h*/ 14'400'000LL,
                                  /*confirm=10*/ 10);
    auto rep = sanitizer.run(stream);
    std::cout << "--- Sanitize report ---\n";
    std::cout << "ok                   : " << (rep.ok ? "YES" : "NO") << "\n";
    std::cout << "bars_scanned         : " << rep.bars_scanned << "\n";
    std::cout << "bars_skipped         : " << rep.bars_skipped << "\n";
    std::cout << "max_gap_seen_ms      : " << rep.max_gap_seen_ms
              << "  (" << (rep.max_gap_seen_ms / 86'400'000LL) << " days)\n";
    if (rep.first_regular_timestamp > 0) {
        std::time_t t = static_cast<std::time_t>(rep.first_regular_timestamp / 1000);
        char buf[64];
        std::tm* tm_utc = std::gmtime(&t);
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S UTC", tm_utc);
        std::cout << "first_regular_time   : " << buf << "\n";
    }
    // --- Читаем первые 5 баров после санитайза ---
    std::cout << "\n--- First 5 bars after sanitize ---\n";
    std::cout << std::fixed << std::setprecision(5);
    core::Bar b;
    int n = 0;
    while (n < 5 && stream.next(b)) {
        ++n;
        std::time_t t = static_cast<std::time_t>(b.timestamp / 1000);
        char buf[64];
        std::tm* tm_utc = std::gmtime(&t);
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm_utc);
        std::cout << "Bar #" << std::setw(2) << n
                  << "  " << buf
                  << "  O=" << b.open
                  << "  H=" << b.high
                  << "  L=" << b.low
                  << "  C=" << b.close
                  << "  spr=" << b.spread
                  << "\n";
    }
    std::cout << "\nbars emitted so far  : " << stream.bars_read() << "\n";
    std::cout << "Done.\n";
    return 0;
}