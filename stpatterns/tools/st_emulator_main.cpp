// =============================================================================
//  STPatterns :: st_emulator_main.cpp
//  Scans a directory for *_H1.bin, prints per-symbol and bucket stats.
// =============================================================================
#include "data/BarStream.h"
#include "core/Types.h"
#include "st/fractal.h"
#include "st/adr.h"
#include "st/corridor.h"
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;
struct Stats {
    int n = 0, w = 0, l = 0, be = 0;
    double sum_win = 0.0, sum_loss = 0.0, net = 0.0;
};
static void add_trade(Stats& s, const st::CorridorEvent& e)
{
    s.n++;
    s.net += e.pnl_pts;
    if (e.exit_reason == "tp")      { s.w++; s.sum_win += e.pnl_pts; }
    else if (e.exit_reason == "sl") { s.l++; s.sum_loss += -e.pnl_pts; }
    else if (e.exit_reason == "be") { s.be++; }
}
static double wr_of(const Stats& s)
{
    return (s.w + s.l > 0) ? 100.0 * s.w / (s.w + s.l) : 0.0;
}
static double pf_of(const Stats& s)
{
    return (s.sum_loss > 0.0) ? s.sum_win / s.sum_loss : 0.0;
}
static double avg_of(const Stats& s)
{
    return (s.n > 0) ? s.net / s.n : 0.0;
}
static Stats run_one(const std::string& path,
                     std::vector<st::CorridorEvent>& out_all)
{
    Stats s;
    spartak::data::BarStream stream(path, 8192);
    if (!stream.is_ok()) return s;
    std::vector<spartak::core::Bar> bars;
    spartak::core::Bar b;
    while (stream.next(b)) bars.push_back(b);
    if (bars.empty()) return s;
    auto fr    = st::find_fractals(bars);
    auto daily = st::build_daily(bars);
    double point     = 0.00001;
    double threshold = 70.0;
    double tp_mult   = 4.0;
    auto sig = st::scan_corridors(bars, fr, daily, point,
                                  threshold, true, 0.5, tp_mult);
    for (auto& e : sig) {
        add_trade(s, e);
        out_all.push_back(e);
    }
    return s;
}
int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: st_emulator <dir_or_file>\n");
        return 1;
    }
    const std::string arg = argv[1];
    std::vector<std::string> files;
    if (fs::is_directory(arg)) {
        for (auto& e : fs::recursive_directory_iterator(arg)) {
            if (!e.is_regular_file()) continue;
            std::string name = e.path().filename().string();
            if (name.size() < 7) continue;
            if (name.substr(name.size() - 7) == "_H1.bin")
                files.push_back(e.path().string());
        }
        std::sort(files.begin(), files.end());
    } else {
        files.push_back(arg);
    }
    std::printf("files: %zu\n\n", files.size());
    std::vector<st::CorridorEvent> all;
    std::printf("%-12s %6s %6s %6s %6s %6s %8s %8s\n",
                "symbol", "n", "W", "L", "BE", "WR%", "PF", "avg");
    Stats total;
    for (auto& f : files) {
        std::string sym = fs::path(f).parent_path().filename().string();
        Stats s = run_one(f, all);
        for (auto& e : all) { (void)e; }
        total.n += s.n; total.w += s.w; total.l += s.l; total.be += s.be;
        total.sum_win += s.sum_win; total.sum_loss += s.sum_loss; total.net += s.net;
        std::printf("%-12s %6d %6d %6d %6d %6.1f %8.2f %8.1f\n",
                    sym.c_str(), s.n, s.w, s.l, s.be,
                    wr_of(s), pf_of(s), avg_of(s));
    }
    std::printf("\n%-12s %6d %6d %6d %6d %6.1f %8.2f %8.1f\n",
                "TOTAL", total.n, total.w, total.l, total.be,
                wr_of(total), pf_of(total), avg_of(total));
    // Bucket breakdown over all events.
    Stats b1, b2, b3, b4;
    for (auto& e : all) {
        if (e.adr_pct < 20)      add_trade(b1, e);
        else if (e.adr_pct < 30) add_trade(b2, e);
        else if (e.adr_pct < 40) add_trade(b3, e);
        else                     add_trade(b4, e);
    }
    std::printf("\n=== bucket by corridor size (pct of ADR) ===\n");
    std::printf("  %-8s n=%5d W=%4d L=%5d BE=%4d WR=%5.1f%% PF=%.2f avg=%6.1f\n",
                "0-20",   b1.n, b1.w, b1.l, b1.be, wr_of(b1), pf_of(b1), avg_of(b1));
    std::printf("  %-8s n=%5d W=%4d L=%5d BE=%4d WR=%5.1f%% PF=%.2f avg=%6.1f\n",
                "20-30",  b2.n, b2.w, b2.l, b2.be, wr_of(b2), pf_of(b2), avg_of(b2));
    std::printf("  %-8s n=%5d W=%4d L=%5d BE=%4d WR=%5.1f%% PF=%.2f avg=%6.1f\n",
                "30-40",  b3.n, b3.w, b3.l, b3.be, wr_of(b3), pf_of(b3), avg_of(b3));
    std::printf("  %-8s n=%5d W=%4d L=%5d BE=%4d WR=%5.1f%% PF=%.2f avg=%6.1f\n",
                "40-50",  b4.n, b4.w, b4.l, b4.be, wr_of(b4), pf_of(b4), avg_of(b4));
    return 0;
}