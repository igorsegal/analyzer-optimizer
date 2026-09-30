// Smoke: HighExtractor + LowExtractor на реальном M5.
// Проверяем, что разделение фракталов по типу сохраняет порядок
// и не теряет бары.
#include "data/BarStream.h"
#include "data/DataSanitizer.h"
#include "context/FractalPointDetector.h"
#include "context/HighExtractor.h"
#include "context/LowExtractor.h"
#include <iostream>
#include <iomanip>
#include <vector>
int main(int argc, char** argv) {
    using namespace spartak;
    const std::string path = (argc > 1)
        ? argv[1]
        : "D:/AHexaTrader/1DataFiles/raw/EURUSD/EURUSD_M5_real.bin";
    std::cout << "=== High/Low Extractor smoke test ===\n\n";
    data::BarStream stream(path);
    if (!stream.is_ok()) { std::cout << "Stream open FAILED\n"; return 1; }
    data::DataSanitizer sanitizer;
    sanitizer.run(stream);
    constexpr int N = 2000;
    std::vector<core::Bar> bars;
    bars.reserve(N);
    core::Bar b;
    while (bars.size() < N && stream.next(b)) bars.push_back(b);
    context::FractalPointDetector det(2);
    auto all = det.find(bars);
    auto highs = context::HighExtractor::extract(all);
    auto lows  = context::LowExtractor::extract(all);
    std::cout << "Total extrema   : " << all.size()   << "\n";
    std::cout << "Highs extracted : " << highs.size() << "\n";
    std::cout << "Lows  extracted : " << lows.size()  << "\n";
    std::cout << "Sum check       : "
              << (highs.size() + lows.size() == all.size() ? "OK" : "MISMATCH")
              << "\n\n";
    // Проверка: все элементы — действительно нужного типа
    bool ok = true;
    for (const auto& e : highs) if (e.kind != context::Extremum::Kind::High) ok = false;
    for (const auto& e : lows)  if (e.kind != context::Extremum::Kind::Low)  ok = false;
    std::cout << "Type integrity  : " << (ok ? "OK" : "BROKEN") << "\n\n";
    // Хронология (индексы возрастают)
    bool chrono = true;
    for (size_t i = 1; i < highs.size(); ++i)
        if (highs[i].index <= highs[i-1].index) chrono = false;
    for (size_t i = 1; i < lows.size(); ++i)
        if (lows[i].index <= lows[i-1].index) chrono = false;
    std::cout << "Chronology      : " << (chrono ? "OK" : "BROKEN") << "\n\n";
    // Первые 5 High и 5 Low — визуально
    std::cout << "--- First 5 Highs ---\n";
    std::cout << std::fixed << std::setprecision(5);
    for (size_t i = 0; i < std::min<size_t>(5, highs.size()); ++i)
        std::cout << "  idx=" << std::setw(4) << highs[i].index
                  << "  " << highs[i].price << "\n";
    std::cout << "\n--- First 5 Lows ---\n";
    for (size_t i = 0; i < std::min<size_t>(5, lows.size()); ++i)
        std::cout << "  idx=" << std::setw(4) << lows[i].index
                  << "  " << lows[i].price << "\n";
    std::cout << "\nDone.\n";
    return 0;
}