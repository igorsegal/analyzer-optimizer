// Smoke-тест: XFBarReader на реальном XFBAR-файле.
// Открывает .bin, печатает заголовок и первые/последние 5 баров.
#include "data/XFBarReader.h"
#include <iostream>
#include <iomanip>
int main(int argc, char** argv) {
    using namespace spartak;
    const std::string path = (argc > 1)
        ? argv[1]
        : "D:/AHexaTrader/1DataFiles/raw/EURUSD/EURUSD_M5.bin";
    std::cout << "=== XFBarReader smoke test ===\n\n";
    std::cout << "Path: " << path << "\n\n";
    data::XFBarReader reader(8192);
    if (!reader.open(path)) {
        std::cout << "OPEN FAILED: status = "
                  << data::to_string(reader.status()) << "\n";
        return 1;
    }
    const auto& h = reader.header();
    std::cout << "--- Header ---\n";
    std::cout << "magic        : " << std::string(h.magic, 8) << "\n";
    std::cout << "version      : " << h.version        << "\n";
    std::cout << "record_size  : " << h.record_size    << "\n";
    std::cout << "period_sec   : " << h.period_seconds << "  ("
              << (h.period_seconds / 60) << " min)\n";
    std::cout << "digits       : " << h.digits         << "\n";
    std::cout << std::fixed << std::setprecision(8);
    std::cout << "point        : " << h.point          << "\n";
    std::cout << "bar_count    : " << h.bar_count      << "\n";
    std::cout << "first_time   : " << h.first_time     << "\n";
    std::cout << "last_time    : " << h.last_time      << "\n";
    std::cout << "symbol_len   : " << h.symbol_len     << "\n";
    std::cout << "symbol       : " << h.symbol         << "\n\n";
    // --- Первые 5 баров ---
    std::cout << "--- First 5 bars ---\n";
    core::Bar b;
    int n = 0;
    while (n < 5 && reader.read_next_bar(b)) {
        ++n;
        std::cout << std::setprecision(5);
        std::cout << "Bar #" << std::setw(2) << n
                  << "  t=" << b.timestamp
                  << "  O=" << b.open
                  << "  H=" << b.high
                  << "  L=" << b.low
                  << "  C=" << b.close
                  << "  vol=" << b.tick_volume
                  << "  spr=" << b.spread
                  << "\n";
    }
    // --- Прокрутка до конца (без вывода) ---
    int64_t total_seen = n;
    while (reader.read_next_bar(b)) { ++total_seen; }
    std::cout << "\n--- Total read ---\n";
    std::cout << "bars read        : " << total_seen << "\n";
    std::cout << "expected count   : " << h.bar_count << "\n";
    std::cout << "match            : "
              << (total_seen == h.bar_count ? "YES" : "NO") << "\n";
    std::cout << "reader status    : "
              << data::to_string(reader.status()) << "\n";
    // --- Последний бар ---
    std::cout << "\n--- Last bar ---\n";
    std::cout << std::setprecision(5);
    std::cout << "t="   << b.timestamp
              << "  O=" << b.open
              << "  H=" << b.high
              << "  L=" << b.low
              << "  C=" << b.close
              << "  vol=" << b.tick_volume
              << "  spr=" << b.spread
              << "\n";
    std::cout << "\nDone.\n";
    return 0;
}