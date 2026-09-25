// Smoke-тест: FractalPointDetector на реальном XFBAR-файле.
// Считает фракталы radius=2 на префиксе 2000 баров (регулярный M5).
#include "data/BarStream.h"
#include "data/DataSanitizer.h"
#include "context/FractalPointDetector.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <ctime>
int main(int argc, char** argv) {
    using namespace spartak;
    const std::string path = (argc > 1)
        ? argv[1]
        : "D:/AHexaTrader/1DataFiles/raw/EURUSD/EURUSD_M5.bin";
    std::cout << "=== FractalPointDetector smoke test ===\n\n";
    std::cout << "Path: " << path << "\n\n";
    data::BarStream stream(path);
    if (!stream.is_ok()) {
        std::cout << "Stream open FAILED\n";
        return 1;
    }
    // Пропускаем нерегулярный префикс
    data::DataSanitizer sanitizer;
    auto rep = sanitizer.run(stream);
    std::cout << "Sanitizer: skipped " << rep.bars_skipped
              << " bars, ok=" << (rep.ok ? "YES" : "NO") << "\n\n";
    // Берём префикс 2000 баров для анализа
    constexpr int N = 2000;
    std::vector<core::Bar> bars;
    bars.reserve(N);
    core::Bar b;
    while (bars.size() < N && stream.next(b)) {
        bars.push_back(b);
    }
    std::cout << "Collected bars: " << bars.size() << "\n\n";
    // --- Fractal detector ---
    context::FractalPointDetector det(/*radius=*/2);
    auto extrema = det.find(bars);
    // Считаем High/Low
    int n_high = 0, n_low = 0;
    for (const auto& e : extrema) {
        if (e.kind == context::Extremum::Kind::High) ++n_high;
        else                                          ++n_low;
    }
    std::cout << "--- Fractals (radius=" << det.radius() << ") ---\n";
    std::cout << "total extrema : " << extrema.size() << "\n";
    std::cout << "  High        : " << n_high << "\n";
    std::cout << "  Low         : " << n_low  << "\n\n";
    // --- Первые 10 экстремумов ---
    std::cout << "--- First 10 extrema ---\n";
    std::cout << std::fixed << std::setprecision(5);
    int shown = 0;
    for (const auto& e : extrema) {
        if (shown >= 10) break;
        ++shown;
        std::time_t t = static_cast<std::time_t>(e.timestamp / 1000);
        char buf[64];
        std::tm* tm_utc = std::gmtime(&t);
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm_utc);
        std::cout << "#" << std::setw(2) << shown
                  << "  idx=" << std::setw(4) << e.index
                  << "  " << buf
                  << "  " << (e.kind == context::Extremum::Kind::High ? "HIGH" : "LOW ")
                  << "  " << e.price
                  << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}