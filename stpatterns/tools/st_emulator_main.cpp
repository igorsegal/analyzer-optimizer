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
#include <map>
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
static void print_stats(const char* tag, const Stats& s)
{
    double wr = (s.w + s.l > 0) ? 100.0 * s.w / (s.w + s.l) : 0.0;
    double pf = (s.sum_loss > 0.0) ? s.sum_win / s.sum_loss : 0.0;
    double avg = (s.n > 0) ? s.net / s.n : 0.0;
    std::printf("  %-14s  n=%5d  W=%4d  L=%5d  BE=%4d  WR=%5.1f%%  PF=%.2f  net=%8.0f  avg=%6.1f\n",
                tag, s.n, s.w, s.l, s.be, wr, pf, s.net, avg);
}
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
    auto fr    = st::find_fractals(bars);
    auto daily = st::build_daily(bars);
    double point     = 0.00001;
    double threshold = 70.0;
    double tp_mult   = 4.0;
    auto sig = st::scan_corridors(bars, fr, daily, point,
                                  threshold, true, 0.5, tp_mult);
    std::printf("file  : %s\n", path.c_str());
    std::printf("bars  : %zu\n", bars.size());
    std::printf("trades: %zu\n", sig.size());
    Stats all;
    for (auto& e : sig) add_trade(all, e);
    std::printf("\n=== overall ===\n");
    print_stats("ALL", all);
    Stats buy, sell;
    for (auto& e : sig) {
        if (e.dir > 0) add_trade(buy, e);
        else           add_trade(sell, e);
    }
    std::printf("\n=== by direction ===\n");
    print_stats("BUY",  buy);
    print_stats("SELL", sell);
    std::printf("\n=== previous direction stats ===\n");
    int pd_plus = 0, pd_minus = 0, pd_zero = 0;
    int same = 0, diff = 0;
    int nff_used = 0;
    for (auto& e : sig) {
        if (e.prev_dir_val == +1) ++pd_plus;
        else if (e.prev_dir_val == -1) ++pd_minus;
        else ++pd_zero;
        if (e.prev_dir_val == e.dir) ++same;
        else ++diff;
        if (e.used_nff == 1) ++nff_used;
    }
    std::printf("  prev_dir +1 : %d\n", pd_plus);
    std::printf("  prev_dir -1 : %d\n", pd_minus);
    std::printf("  prev_dir  0 : %d\n", pd_zero);
    std::printf("  same as trade dir : %d\n", same);
    std::printf("  not-fully-formed stop used : %d\n", nff_used);
    std::printf("\n=== by year ===\n");
    std::map<int, Stats> by_year;
    for (auto& e : sig) add_trade(by_year[e.year], e);
    for (auto& kv : by_year) {
        char tag[16];
        std::snprintf(tag, sizeof(tag), "%d", kv.first);
        print_stats(tag, kv.second);
    }
    Stats b1, b2, b3, b4;
    for (auto& e : sig) {
        if (e.adr_pct < 20)      add_trade(b1, e);
        else if (e.adr_pct < 30) add_trade(b2, e);
        else if (e.adr_pct < 40) add_trade(b3, e);
        else                     add_trade(b4, e);
    }
    std::printf("\n=== by corridor size (pct of ADR) ===\n");
    print_stats("0-20",   b1);
    print_stats("20-30",  b2);
    print_stats("30-40",  b3);
    print_stats("40-50",  b4);
    return 0;
}