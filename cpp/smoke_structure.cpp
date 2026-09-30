// Smoke: StructureValidatorHHHL на реальном M5.
// Прогоняет валидатор по скользящему окну 2000 баров и считает,
// как часто структура была Bullish / Bearish / Undefined.
#include "data/BarStream.h"
#include "data/DataSanitizer.h"
#include "context/FractalPointDetector.h"
#include "context/HighExtractor.h"
#include "context/LowExtractor.h"
#include "context/StructureValidatorHHHL.h"
#include <iostream>
#include <iomanip>
#include <vector>
int main(int argc, char** argv) {
    using namespace spartak;
    const std::string path = (argc > 1)
        ? argv[1]
        : "D:/AHexaTrader/1DataFiles/raw/EURUSD/EURUSD_M5_real.bin";
    std::cout << "=== StructureValidatorHHHL smoke test ===\n\n";
    data::BarStream stream(path);
    if (!stream.is_ok()) { std::cout << "Stream open FAILED\n"; return 1; }
    data::DataSanitizer sanitizer;
    (void)sanitizer.run(stream);
    constexpr int N = 2000;
    constexpr std::size_t LOOKBACK = 3;
    std::vector<core::Bar> bars;
    bars.reserve(N);
    core::Bar b;
    while (bars.size() < N && stream.next(b)) bars.push_back(b);
    context::FractalPointDetector det(2);
    auto all   = det.find(bars);
    auto highs = context::HighExtractor::extract(all);
    auto lows  = context::LowExtractor::extract(all);
    std::cout << "Bars          : " << bars.size() << "\n";
    std::cout << "Extrema       : " << all.size()
              << "  (H=" << highs.size()
              << ", L=" << lows.size() << ")\n";
    std::cout << "Lookback      : " << LOOKBACK << "\n\n";
    // --- Полная проверка на всём окне ---
    auto st = context::StructureValidatorHHHL::validate(highs, lows, LOOKBACK);
    const char* name = "Undefined";
    if (st == context::StructureType::Bullish) name = "Bullish";
    if (st == context::StructureType::Bearish) name = "Bearish";
    std::cout << "Full-window structure : " << name << "\n\n";
    // --- Скользящее окно: проверяем каждые 100 баров ---
    // Копим фракталы по мере накопления баров.
    int n_bull = 0, n_bear = 0, n_undef = 0;
    constexpr int STEP = 100;
    for (int window = 300; window <= N; window += STEP) {
        std::vector<core::Bar> slice(bars.begin(), bars.begin() + window);
        auto e = det.find(slice);
        auto h = context::HighExtractor::extract(e);
        auto l = context::LowExtractor::extract(e);
        auto s = context::StructureValidatorHHHL::validate(h, l, LOOKBACK);
        switch (s) {
            case context::StructureType::Bullish: ++n_bull;  break;
            case context::StructureType::Bearish: ++n_bear;  break;
            default:                              ++n_undef; break;
        }
    }
    std::cout << "--- Rolling window (step=" << STEP << ") ---\n";
    std::cout << "  Bullish : " << n_bull  << "\n";
    std::cout << "  Bearish : " << n_bear  << "\n";
    std::cout << "  Undefined: " << n_undef << "\n";
    std::cout << "  Total   : " << (n_bull + n_bear + n_undef) << "\n\n";
    // --- Ручной тест: синтетика (заведомо Bullish) ---
    {
        std::vector<context::Extremum> h = {
            {context::Extremum::Kind::High, 1, 0, 1.10},
            {context::Extremum::Kind::High, 3, 0, 1.15},
            {context::Extremum::Kind::High, 5, 0, 1.20},
        };
        std::vector<context::Extremum> l = {
            {context::Extremum::Kind::Low, 2, 0, 1.05},
            {context::Extremum::Kind::Low, 4, 0, 1.10},
            {context::Extremum::Kind::Low, 6, 0, 1.15},
        };
        auto s = context::StructureValidatorHHHL::validate(h, l, 3);
        std::cout << "Synthetic HH+HL : "
                  << (s == context::StructureType::Bullish ? "Bullish OK" : "FAIL")
                  << "\n";
    }
    // --- Ручной тест: заведомо Bearish ---
    {
        std::vector<context::Extremum> h = {
            {context::Extremum::Kind::High, 1, 0, 1.20},
            {context::Extremum::Kind::High, 3, 0, 1.15},
            {context::Extremum::Kind::High, 5, 0, 1.10},
        };
        std::vector<context::Extremum> l = {
            {context::Extremum::Kind::Low, 2, 0, 1.15},
            {context::Extremum::Kind::Low, 4, 0, 1.10},
            {context::Extremum::Kind::Low, 6, 0, 1.05},
        };
        auto s = context::StructureValidatorHHHL::validate(h, l, 3);
        std::cout << "Synthetic LL+LH : "
                  << (s == context::StructureType::Bearish ? "Bearish OK" : "FAIL")
                  << "\n";
    }
    // --- Ручной тест: смешанный (Undefined) ---
    {
        std::vector<context::Extremum> h = {
            {context::Extremum::Kind::High, 1, 0, 1.10},
            {context::Extremum::Kind::High, 3, 0, 1.15},
            {context::Extremum::Kind::High, 5, 0, 1.12},
        };
        std::vector<context::Extremum> l = {
            {context::Extremum::Kind::Low, 2, 0, 1.05},
            {context::Extremum::Kind::Low, 4, 0, 1.10},
            {context::Extremum::Kind::Low, 6, 0, 1.08},
        };
        auto s = context::StructureValidatorHHHL::validate(h, l, 3);
        std::cout << "Synthetic mixed : "
                  << (s == context::StructureType::Undefined ? "Undefined OK" : "FAIL")
                  << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}