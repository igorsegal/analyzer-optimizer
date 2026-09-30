// Smoke: ShadowNoiseFilter на реальном M5.
// Сравнивает число экстремумов до и после фильтра.
#include "data/BarStream.h"
#include "data/DataSanitizer.h"
#include "context/FractalPointDetector.h"
#include "context/ShadowNoiseFilter.h"
#include <iostream>
#include <iomanip>
#include <vector>
int main(int argc, char** argv) {
    using namespace spartak;
    const std::string path = (argc > 1)
        ? argv[1]
        : "D:/AHexaTrader/1DataFiles/raw/EURUSD/EURUSD_M5_real.bin";
    std::cout << "=== ShadowNoiseFilter smoke test ===\n\n";
    data::BarStream stream(path);
    if (!stream.is_ok()) { std::cout << "Stream open FAILED\n"; return 1; }
    data::DataSanitizer sanitizer;
    (void)sanitizer.run(stream);
    constexpr int N = 2000;
    std::vector<core::Bar> bars;
    bars.reserve(N);
    core::Bar b;
    while (bars.size() < N && stream.next(b)) bars.push_back(b);
    context::FractalPointDetector det(2);
    auto raw = det.find(bars);
    // --- Без фильтра ---
    int raw_h = 0, raw_l = 0;
    for (const auto& e : raw) {
        if (e.kind == context::Extremum::Kind::High) ++raw_h;
        else                                          ++raw_l;
    }
    // --- С фильтром (по умолчанию) ---
    context::ShadowNoiseConfig cfg;
    cfg.point = 0.00001;
    cfg.confirm_window   = 5;
    cfg.tolerance_points = 2;
    context::ShadowNoiseFilter flt(cfg);
    auto kept = flt.filter(bars, raw);
    int kept_h = 0, kept_l = 0;
    for (const auto& e : kept) {
        if (e.kind == context::Extremum::Kind::High) ++kept_h;
        else                                          ++kept_l;
    }
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Bars         : " << bars.size() << "\n";
    std::cout << "confirm_window = " << cfg.confirm_window
              << ", tolerance = " << cfg.tolerance_points << " pts\n\n";
    std::cout << "--- Raw extrema ---\n";
    std::cout << "  Total: " << raw.size() << "  (H=" << raw_h << ", L=" << raw_l << ")\n";
    std::cout << "\n--- After filter ---\n";
    std::cout << "  Total: " << kept.size() << "  (H=" << kept_h << ", L=" << kept_l << ")\n";
    const double keep_ratio = raw.empty()
        ? 0.0
        : 100.0 * static_cast<double>(kept.size()) / static_cast<double>(raw.size());
    std::cout << "\nKeep ratio   : " << keep_ratio << "%\n";
    const int dropped = static_cast<int>(raw.size() - kept.size());
    std::cout << "Dropped      : " << dropped << "  (noise)\n\n";
    // --- Ручной синтетический тест ---
    // 10 баров: пик на idx=4, потом цена продолжает вверх на idx=6 → шум.
    {
        std::vector<core::Bar> bars2 = {
            {1,  1.0, 1.01, 0.99, 1.005, 0, 0},
            {2,  1.0, 1.02, 0.99, 1.015, 0, 0},
            {3,  1.0, 1.03, 0.99, 1.025, 0, 0},
            {4,  1.0, 1.04, 0.99, 1.035, 0, 0},   // <-- высокий High, но не финальный
            {5,  1.0, 1.035, 0.99, 1.03, 0, 0},
            {6,  1.0, 1.05, 0.99, 1.04, 0, 0},    // <-- пробивает выше idx=4 → шум
            {7,  1.0, 1.04, 0.99, 1.03, 0, 0},
            {8,  1.0, 1.03, 0.99, 1.02, 0, 0},
            {9,  1.0, 1.02, 0.99, 1.01, 0, 0},
            {10, 1.0, 1.01, 0.99, 1.00, 0, 0},
        };
        context::FractalPointDetector d2(1);
        auto raw2 = d2.find(bars2);
        context::ShadowNoiseConfig cfg2;
        cfg2.point = 0.01;
        cfg2.confirm_window = 2;
        cfg2.tolerance_points = 0;
        context::ShadowNoiseFilter f2(cfg2);
        auto kept2 = f2.filter(bars2, raw2);
        std::cout << "--- Synthetic test ---\n";
        std::cout << "  Raw extrema   : " << raw2.size() << "\n";
        std::cout << "  After filter  : " << kept2.size() << "\n";
        std::cout << "  Filter dropped the spoofed top: "
                  << (kept2.size() < raw2.size() ? "YES" : "NO") << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}