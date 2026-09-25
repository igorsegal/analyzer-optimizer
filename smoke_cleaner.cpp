// Smoke: OldLevelCleaner на реальных зонах.
// Проверяет age- и break-критерии.
#include "data/BarStream.h"
#include "data/DataSanitizer.h"
#include "context/FractalPointDetector.h"
#include "context/ShadowNoiseFilter.h"
#include "context/PUZoneCalculator.h"
#include "context/OldLevelCleaner.h"
#include <iostream>
#include <iomanip>
#include <vector>
int main(int argc, char** argv) {
    using namespace spartak;
    const std::string path = (argc > 1)
        ? argv[1]
        : "D:/AHexaTrader/1DataFiles/raw/EURUSD/EURUSD_M5_real.bin";
    std::cout << "=== OldLevelCleaner smoke test ===\n\n";
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
    context::ShadowNoiseConfig scfg;
    scfg.point = 0.00001;
    context::ShadowNoiseFilter flt(scfg);
    auto clean_extrema = flt.filter(bars, raw);
    context::PUZoneCalculator pu(0.00001, 15);
    auto zones = pu.build(clean_extrema, 5);
    std::cout << "Bars       : " << bars.size() << "\n";
    std::cout << "Extrema    : " << clean_extrema.size() << "\n";
    std::cout << "Zones      : " << zones.size() << "\n\n";
    const int64_t now_ms = bars.back().timestamp;
    const double  close  = bars.back().close;
    std::cout << "now_ms      = " << now_ms << "\n";
    std::cout << "close       = " << std::fixed << std::setprecision(5) << close << "\n\n";
    int active_before = 0;
    for (const auto& z : zones) if (z.is_active) ++active_before;
    std::cout << "Active before clean : " << active_before << " / " << zones.size() << "\n";
    context::OldLevelConfig cfg;
    cfg.point = 0.00001;
    cfg.max_age_ms = 7LL * 86'400'000LL;
    cfg.break_buffer_points = 30;
    context::OldLevelCleaner cleaner(cfg);
    auto after = cleaner.clean(zones, now_ms, close);
    int active_after = 0;
    for (const auto& z : after) if (z.is_active) ++active_after;
    std::cout << "Active after clean  : " << active_after << " / " << after.size() << "\n\n";
    std::cout << "--- Zone status ---\n";
    for (std::size_t i = 0; i < after.size(); ++i) {
        const auto& z = after[i];
        const int64_t age_ms = (z.formation_time > 0 && now_ms > z.formation_time)
                             ? (now_ms - z.formation_time) : 0;
        const double age_days = static_cast<double>(age_ms) / 86'400'000.0;
        std::cout << "#" << (i + 1)
                  << "  level=" << z.price_level
                  << "  age=" << std::setprecision(2) << age_days << "d"
                  << "  active=" << (z.is_active ? "Y" : "N") << "\n";
        std::cout << std::setprecision(5);
    }
    std::cout << "\n--- Synthetic tests ---\n";
    {
        std::vector<core::PriceZone> z = {
            {core::LevelType::LocalLevel, now_ms - 3600'000LL,
             1.50000, 1.50010, 1.49990, true}
        };
        auto r = cleaner.clean(z, now_ms, 1.50000);
        std::cout << "Fresh zone, price inside : "
                  << (r[0].is_active ? "ACTIVE (OK)" : "INACTIVE (FAIL)") << "\n";
    }
    {
        std::vector<core::PriceZone> z = {
            {core::LevelType::LocalLevel, now_ms - 10LL * 86'400'000LL,
             1.50000, 1.50010, 1.49990, true}
        };
        auto r = cleaner.clean(z, now_ms, 1.50000);
        std::cout << "Old zone (>7d)           : "
                  << (!r[0].is_active ? "INACTIVE (OK)" : "ACTIVE (FAIL)") << "\n";
    }
    {
        std::vector<core::PriceZone> z = {
            {core::LevelType::LocalLevel, now_ms - 3600'000LL,
             1.50000, 1.50010, 1.49990, true}
        };
        auto r = cleaner.clean(z, now_ms, 1.50070);
        std::cout << "Fresh zone broken up     : "
                  << (!r[0].is_active ? "INACTIVE (OK)" : "ACTIVE (FAIL)") << "\n";
    }
    {
        std::vector<core::PriceZone> z = {
            {core::LevelType::LocalLevel, now_ms - 3600'000LL,
             1.50000, 1.50010, 1.49990, true}
        };
        auto r = cleaner.clean(z, now_ms, 1.49930);
        std::cout << "Fresh zone broken down   : "
                  << (!r[0].is_active ? "INACTIVE (OK)" : "ACTIVE (FAIL)") << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}