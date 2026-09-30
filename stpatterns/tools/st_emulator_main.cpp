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

    double point     = 0.00001;
    double threshold = 70.0;
    double tp_mult   = 4.0;

    auto sig = st::scan_corridors(bars, fr, daily, point,
                                  threshold, true, 0.5, tp_mult);

    std::printf("\ntrades (non-overlap): %zu\n", sig.size());

    int buy = 0, sell = 0;
    int wins = 0, losses = 0, eod = 0;
    double sum_win = 0.0, sum_loss = 0.0, sum_pts = 0.0;
    for (auto& e : sig) {
        if (e.dir > 0) ++buy; else ++sell;
        sum_pts += e.pnl_pts;
        if (e.exit_reason == "tp") { ++wins;   sum_win  += e.pnl_pts; }
        else if (e.exit_reason == "sl") { ++losses; sum_loss += -e.pnl_pts; }
        else { ++eod; }
    }
    std::printf("  buy : %d\n", buy);
    std::printf("  sell: %d\n", sell);
    std::printf("\nresults:\n");
    std::printf("  wins  : %d\n", wins);
    std::printf("  losses: %d\n", losses);
    std::printf("  eod   : %d\n", eod);
    if (wins + losses > 0)
        std::printf("  WR    : %.2f%%\n",
                    100.0 * wins / (wins + losses));
    if (sum_loss > 0.0)
        std::printf("  PF    : %.3f\n", sum_win / sum_loss);
    std::printf("  total : %.0f pts\n", sum_pts);
    std::printf("  avg/trade: %.1f pts\n",
                sig.empty() ? 0.0 : sum_pts / sig.size());

    std::printf("\nfirst 5 trades:\n");
    for (int i = 0; i < (int)sig.size() && i < 5; ++i) {
        auto& e = sig[i];
        std::printf("  #%d %s  entry_idx=%d exit_idx=%d  entry=%.5f exit=%.5f  %s  pnl=%.0f pts\n",
                    i, (e.dir > 0 ? "BUY " : "SELL"),
                    e.entry_idx, e.exit_idx,
                    e.entry_price, e.exit_price,
                    e.exit_reason.c_str(), e.pnl_pts);
    }
    std::printf("\nlast 5 trades:\n");
    int s = (int)sig.size() - 5; if (s < 0) s = 0;
    for (int i = s; i < (int)sig.size(); ++i) {
        auto& e = sig[i];
        std::printf("  #%d %s  entry_idx=%d exit_idx=%d  entry=%.5f exit=%.5f  %s  pnl=%.0f pts\n",
                    i, (e.dir > 0 ? "BUY " : "SELL"),
                    e.entry_idx, e.exit_idx,
                    e.entry_price, e.exit_price,
                    e.exit_reason.c_str(), e.pnl_pts);
    }
    return 0;
}