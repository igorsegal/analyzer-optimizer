// =============================================================================
//  STPatterns :: st_emulator_main.cpp
//  Read XFBAR, detect fractals, print summary.
// =============================================================================
#include "data/BarStream.h"
#include "core/Types.h"
#include "st/fractal.h"
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

    auto fr = st::find_fractals(bars);

    int up = 0, dn = 0;
    for (auto& f : fr) { if (f.type > 0) ++up; else ++dn; }

    std::printf("total : %zu fractals (up=%d dn=%d)\n", fr.size(), up, dn);
    std::printf("rate  : %.2f fractals / 100 bars\n",
                100.0 * fr.size() / bars.size());
    std::printf("\nfirst 5 fractals:\n");
    for (int i = 0; i < (int)fr.size() && i < 5; ++i) {
        std::printf("  #%d %s idx=%d  price=%.5f\n",
                    i, (fr[i].type > 0) ? "UP  " : "DOWN",
                    fr[i].bar_idx, fr[i].price);
    }
    std::printf("\nlast 5 fractals:\n");
    int start = (int)fr.size() - 5;
    if (start < 0) start = 0;
    for (int i = start; i < (int)fr.size(); ++i) {
        std::printf("  #%d %s idx=%d  price=%.5f\n",
                    i, (fr[i].type > 0) ? "UP  " : "DOWN",
                    fr[i].bar_idx, fr[i].price);
    }
    return 0;
}