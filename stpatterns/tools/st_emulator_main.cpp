// =============================================================================
//  STPatterns :: st_emulator_main.cpp
//  Read XFBAR, detect fractals, build daily, print ADR(5).
// =============================================================================
#include "data/BarStream.h"
#include "core/Types.h"
#include "st/fractal.h"
#include "st/adr.h"
#include <cstdio>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: st_emulator <file.bin>\n");
        return 1;
    }
    const std::string path = argv[1];

    spartak::data::BarStream stream(path, 8192);
    if (!stream.is_ok()) {
        std::fprintf(stderr, "cannot open: %s\n", path.c_str());
        return 1;
    }

    std::vector<spartak::core::Bar> bars;
    spartak::core::Bar b;
    while (stream.next(b)) bars.push_back(b);

    std::printf("file  : %s\n", path.c_str());
    std::printf("bars  : %zu\n", bars.size());
    if (bars.empty()) return 1;

    // --- Fractals ---
    auto fr = st::find_fractals(bars);
    int up = 0, dn = 0;
    for (auto& f : fr) { if (f.type > 0) ++up; else ++dn; }
    std::printf("fract : %zu total (up=%d dn=%d) = %.2f per 100 bars\n",
                fr.size(), up, dn, 100.0 * fr.size() / bars.size());

    // --- Daily + ADR ---
    auto daily = st::build_daily(bars);
    std::printf("daily : %zu days\n", daily.size());

    std::printf("\nADR(5) sample (10 days starting from day 20):\n");
    for (int i = 20; i < (int)daily.size() && i < 30; ++i) {
        double a = st::adr5_at(daily, i);
        std::printf("  day %4d  key=%08d  high=%.5f  low=%.5f  range=%.5f  ADR5=%.5f\n",
                    i, daily[i].day_key, daily[i].high, daily[i].low,
                    daily[i].high - daily[i].low, a);
    }

    std::printf("\nlast 5 days:\n");
    int start = (int)daily.size() - 5;
    if (start < 0) start = 0;
    for (int i = start; i < (int)daily.size(); ++i) {
        double a = st::adr5_at(daily, i);
        std::printf("  day %4d  key=%08d  high=%.5f  low=%.5f  range=%.5f  ADR5=%.5f\n",
                    i, daily[i].day_key, daily[i].high, daily[i].low,
                    daily[i].high - daily[i].low, a);
    }
    return 0;
}