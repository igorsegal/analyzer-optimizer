// =============================================================================
//  STPatterns :: st_emulator_main.cpp
// =============================================================================
#include "data/BarStream.h"
#include "core/Types.h"
#include "st/fractal.h"
#include "st/adr.h"
#include "st/corridor.h"
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
    if (bars.empty()) return 1;

    std::printf("file  : %s\n", path.c_str());
    std::printf("bars  : %zu\n", bars.size());

    auto fr    = st::find_fractals(bars);
    auto daily = st::build_daily(bars);

    std::printf("fract : %zu\n", fr.size());
    std::printf("daily : %zu\n", daily.size());

    // Point from first bar (approx).
    double point = 0.00001;   // EURUSD M5/H1 5 digits
    double threshold = 70.0;

    auto sig = st::scan_corridors(bars, fr, daily, point, threshold, true, 0.5);
    std::printf("\nsignals (ADR filter on): %zu\n", sig.size());

    int buy = 0, sell = 0;
    for (auto& e : sig) { if (e.dir > 0) ++buy; else ++sell; }
    std::printf("  buy : %d\n", buy);
    std::printf("  sell: %d\n", sell);

    std::printf("\nfirst 10 signals:\n");
    for (int i = 0; i < (int)sig.size() && i < 10; ++i) {
        auto& e = sig[i];
        std::printf("  #%d %s idx=%d  entry=%.5f  sl=%.5f  h=%d pts\n",
                    i, (e.dir > 0 ? "BUY " : "SELL"),
                    e.entry_idx, e.entry_price, e.stop_line, e.height_pts);
    }

    std::printf("\nlast 5 signals:\n");
    int s = (int)sig.size() - 5;
    if (s < 0) s = 0;
    for (int i = s; i < (int)sig.size(); ++i) {
        auto& e = sig[i];
        std::printf("  #%d %s idx=%d  entry=%.5f  sl=%.5f  h=%d pts\n",
                    i, (e.dir > 0 ? "BUY " : "SELL"),
                    e.entry_idx, e.entry_price, e.stop_line, e.height_pts);
    }
    return 0;
}