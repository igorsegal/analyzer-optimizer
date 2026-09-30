// =============================================================================
//  STPatterns :: st_emulator_main.cpp
//  Per-year + JPY/Gold split.
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
#include <set>
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;
static const std::set<std::string> g_universe = {
    "EURUSD", "GBPUSD", "USDJPY", "USDCHF", "USDCAD", "AUDUSD", "NZDUSD",
    "EURGBP", "EURJPY", "EURCHF", "EURAUD", "EURCAD", "EURNZD",
    "GBPJPY", "GBPCHF", "GBPAUD", "GBPCAD", "GBPNZD",
    "AUDJPY", "AUDCHF", "AUDCAD", "AUDNZD",
    "NZDJPY", "NZDCHF", "NZDCAD",
    "CADJPY", "CADCHF", "CHFJPY",
    "XAUUSD", "XAGUSD", "XAUEUR", "XAUAUD", "XAUJPY", "XAGAUD"
};
static bool is_jpy_or_gold(const std::string& s)
{
    if (s.size() >= 6 && (s.substr(3,3) == "JPY" || s.substr(0,3) == "JPY"))
        return true;
    if (s.rfind("XAU",0) == 0) return true;
    if (s.rfind("XAG",0) == 0) return true;
    return false;
}
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
{ return (s.w + s.l > 0) ? 100.0 * s.w / (s.w + s.l) : 0.0; }
static double pf_of(const Stats& s)
{ return (s.sum_loss > 0.0) ? s.sum_win / s.sum_loss : 0.0; }
static double avg_pts(const Stats& s)
{ return (s.n > 0) ? s.net / s.n : 0.0; }
static void print_stats(const char* tag, const Stats& s)
{
    std::printf("  %-14s  n=%6d  W=%5d  L=%6d  BE=%5d  WR=%5.1f%%  PF=%.2f  avg=%7.1f\n",
                tag, s.n, s.w, s.l, s.be, wr_of(s), pf_of(s), avg_pts(s));
}
static void run_one(const std::string& path,
                    const std::string& sym,
                    std::vector<st::CorridorEvent>& out_all)
{
    spartak::data::BarStream stream(path, 8192);
    if (!stream.is_ok()) return;
    const auto* rd = stream.reader();
    if (!rd) return;
    double point = rd->header().point;
    if (point <= 0.0) point = 0.00001;
    std::vector<spartak::core::Bar> bars;
    spartak::core::Bar b;
    while (stream.next(b)) bars.push_back(b);
    if (bars.empty()) return;
    auto fr    = st::find_fractals(bars);
    auto daily = st::build_daily(bars);
    auto sig = st::scan_corridors(bars, fr, daily, point, true, 0.5, 4.0);
    for (auto& e : sig) out_all.push_back(e);
    (void)sym;
}
int main(int argc, char** argv)
{
    if (argc < 2) { std::fprintf(stderr, "usage: st_emulator <dir>\n"); return 1; }
    const std::string arg = argv[1];
    std::vector<std::pair<std::string,std::string>> files;  // (sym, path)
    for (auto& e : fs::recursive_directory_iterator(arg)) {
        if (!e.is_regular_file()) continue;
        std::string name = e.path().filename().string();
        if (name.size() < 7) continue;
        if (name.substr(name.size() - 7) != "_H1.bin") continue;
        std::string sym = e.path().parent_path().filename().string();
        if (g_universe.count(sym) == 0) continue;
        files.emplace_back(sym, e.path().string());
    }
    std::sort(files.begin(), files.end());
    std::printf("files: %zu\n", files.size());
    std::vector<st::CorridorEvent> all;
    for (auto& [sym, path] : files) run_one(path, sym, all);
    std::printf("total trades: %zu\n\n", all.size());
    // --- By symbol group ---
    Stats g1, g2;
    for (auto& e : all) {
        // can't know symbol from event; we print aggregated instead
        (void)e;
    }
    // Since CorridorEvent has no symbol, aggregate using per-symbol runs:
    // re-run each symbol and add to right group.
    Stats jpy_gold, other;
    for (auto& [sym, path] : files) {
        std::vector<st::CorridorEvent> evs;
        run_one(path, sym, evs);
        for (auto& e : evs) {
            if (is_jpy_or_gold(sym)) add_trade(jpy_gold, e);
            else                     add_trade(other, e);
        }
    }
    std::printf("=== by group ===\n");
    print_stats("JPY + GOLD", jpy_gold);
    print_stats("other FX",   other);
    // --- Per-year for JPY+Gold ---
    std::map<int, Stats> by_year_jg;
    std::map<int, Stats> by_year_ot;
    for (auto& [sym, path] : files) {
        std::vector<st::CorridorEvent> evs;
        run_one(path, sym, evs);
        bool is_jg = is_jpy_or_gold(sym);
        for (auto& e : evs) {
            if (is_jg) add_trade(by_year_jg[e.year], e);
            else       add_trade(by_year_ot[e.year], e);
        }
    }
    std::printf("\n=== JPY + GOLD by year ===\n");
    for (auto& kv : by_year_jg) {
        char tag[16];
        std::snprintf(tag, sizeof(tag), "%d", kv.first);
        print_stats(tag, kv.second);
    }
    std::printf("\n=== other FX by year ===\n");
    for (auto& kv : by_year_ot) {
        char tag[16];
        std::snprintf(tag, sizeof(tag), "%d", kv.first);
        print_stats(tag, kv.second);
    }
    return 0;
}