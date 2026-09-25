// Smoke: PUZoneCalculator + LUZoneCalculator на реальном M5.
// Строит зоны из отфильтрованных экстремумов и печатает первые 5.
#include "data/BarStream.h"
#include "data/DataSanitizer.h"
#include "context/FractalPointDetector.h"
#include "context/ShadowNoiseFilter.h"
#include "context/PUZoneCalculator.h"
#include "context/LUZoneCalculator.h"
#include <iostream>
#include <iomanip>
#include <vector>
int main(int argc, char** argv) {
    using namespace spartak;
    const std::string path = (argc > 1)
        ? argv[1]
        : "D:/AHexaTrader/1DataFiles/raw/EURUSD/EURUSD_M5_real.bin";
    std::cout << "=== PU / LU ZoneCalculator smoke test ===\n\n";
    data::BarStream stream(path);
    if (!stream.is_ok()) { std::cout << "Stream open FAILED\n"; return 1; }
    data::DataSanitizer sanitizer;
    (void)sanitizer.run(stream);
    constexpr int N = 2000;
    std::vector<core::Bar> bars;
    bars.reserve(N);
    core::Bar b;
    while (bars.size() < N && stream.next(b)) bars.push_back(b);
    // --- Пайплайн: фракталы -> фильтр шума ---
    context::FractalPointDetector det(2);
    auto raw = det.find(bars);
    context::ShadowNoiseConfig scfg;
    scfg.point = 0.00001;
    scfg.confirm_window = 5;
    scfg.tolerance_points = 2;
    context::ShadowNoiseFilter flt(scfg);
    auto clean = flt.filter(bars, raw);
    std::cout << "Bars              : " << bars.size() << "\n";
    std::cout << "Raw extrema       : " << raw.size()   << "\n";
    std::cout << "After noise filter: " << clean.size() << "\n\n";
    // --- PU-зоны (для теста — на M5, но с маркером IntermediateLevel) ---
    context::PUZoneCalculator pu(0.00001, /*offset=*/15);
    auto pu_zones = pu.build(clean, /*max_zones=*/5);
    std::cout << "--- First 5 PU zones (offset=" << pu.offsetPoints() << " pts) ---\n";
    std::cout << std::fixed << std::setprecision(5);
    for (std::size_t i = 0; i < pu_zones.size(); ++i) {
        const auto& z = pu_zones[i];
        std::cout << "#" << (i + 1)
                  << "  type=" << (z.type == core::LevelType::IntermediateLevel ? "PU" : "LU")
                  << "  level=" << z.price_level
                  << "  zone=[" << z.zone_bottom << " .. " << z.zone_top << "]"
                  << "  active=" << (z.is_active ? "Y" : "N")
                  << "\n";
    }
    // --- LU-зоны ---
    context::LUZoneCalculator lu(0.00001, /*offset=*/10);
    auto lu_zones = lu.build(clean, /*max_zones=*/5);
    std::cout << "\n--- First 5 LU zones (offset=" << lu.offsetPoints() << " pts) ---\n";
    for (std::size_t i = 0; i < lu_zones.size(); ++i) {
        const auto& z = lu_zones[i];
        std::cout << "#" << (i + 1)
                  << "  type=" << (z.type == core::LevelType::LocalLevel ? "LU" : "PU")
                  << "  level=" << z.price_level
                  << "  zone=[" << z.zone_bottom << " .. " << z.zone_top << "]"
                  << "  active=" << (z.is_active ? "Y" : "N")
                  << "\n";
    }
    // --- Проверки ---
    bool ok = true;
    for (const auto& z : pu_zones)
        if (z.type != core::LevelType::IntermediateLevel) ok = false;
    for (const auto& z : lu_zones)
        if (z.type != core::LevelType::LocalLevel) ok = false;
    const double pu_width = pu_zones.empty() ? 0.0
                           : (pu_zones[0].zone_top - pu_zones[0].zone_bottom);
    const double lu_width = lu_zones.empty() ? 0.0
                           : (lu_zones[0].zone_top - lu_zones[0].zone_bottom);
    const double pu_expected = 2.0 * 15 * 0.00001;
    const double lu_expected = 2.0 * 10 * 0.00001;
    std::cout << "\n--- Checks ---\n";
    std::cout << "PU type correct     : " << (ok ? "OK" : "FAIL") << "\n";
    std::cout << "PU zone width       : " << pu_width
              << "  (expect " << pu_expected << ") "
              << (std::fabs(pu_width - pu_expected) < 1e-9 ? "OK" : "FAIL") << "\n";
    std::cout << "LU zone width       : " << lu_width
              << "  (expect " << lu_expected << ") "
              << (std::fabs(lu_width - lu_expected) < 1e-9 ? "OK" : "FAIL") << "\n";
    std::cout << "PU zones count      : " << pu_zones.size() << " (cap=5) "
              << (pu_zones.size() <= 5 ? "OK" : "FAIL") << "\n";
    std::cout << "LU zones count      : " << lu_zones.size() << " (cap=5) "
              << (lu_zones.size() <= 5 ? "OK" : "FAIL") << "\n";
    std::cout << "\nDone.\n";
    return 0;
}