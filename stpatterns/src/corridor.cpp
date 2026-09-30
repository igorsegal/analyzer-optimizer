// =============================================================================
//  STPatterns :: st/corridor.cpp
// =============================================================================
#include "st/corridor.h"
#include <cmath>
#include <ctime>
#include <algorithm>
namespace st {
static int day_key_from_ms(int64_t ms)
{
    std::time_t secs = (std::time_t)(ms / 1000);
    std::tm tm{};
#if defined(_MSC_VER)
    gmtime_s(&tm, &secs);
#else
    gmtime_r(&secs, &tm);
#endif
    return (tm.tm_year + 1900) * 10000 + (tm.tm_mon + 1) * 100 + tm.tm_mday;
}
static int year_from_ms(int64_t ms)
{
    std::time_t secs = (std::time_t)(ms / 1000);
    std::tm tm{};
#if defined(_MSC_VER)
    gmtime_s(&tm, &secs);
#else
    gmtime_r(&secs, &tm);
#endif
    return tm.tm_year + 1900;
}
static int find_day_idx(const std::vector<DailyBar>& daily, int day_key)
{
    for (int i = 0; i < (int)daily.size(); ++i)
        if (daily[i].day_key == day_key) return i;
    return -1;
}
static int find_stop_fractal(const std::vector<Fractal>& fractals,
                             int curr_idx,
                             int dir)
{
    const Fractal& curr = fractals[curr_idx];
    for (int k = curr_idx - 1; k >= 0; --k) {
        const Fractal& f = fractals[k];
        if (!f.fully_formed) continue;
        if (dir > 0) {
            if (f.type == -1 && f.price < curr.price) return k;
        } else {
            if (f.type == +1 && f.price > curr.price) return k;
        }
    }
    return -1;
}
// Previous direction based on the LAST fractal before the corridor.
// If the last fractal is UP  -> check UP pair (HH => +1).
// If the last fractal is DOWN-> check DOWN pair (LL => -1).
// Otherwise 0.
static int find_previous_direction(const std::vector<Fractal>& fractals,
                                   int curr_idx)
{
    // Find the most recent fully-formed fractal before the corridor.
    int last_idx = -1;
    for (int k = curr_idx - 1; k >= 0; --k) {
        if (fractals[k].fully_formed) { last_idx = k; break; }
    }
    if (last_idx < 0) return 0;
    int last_type = fractals[last_idx].type;
    if (last_type > 0) {
        // Find previous UP fractal before last_idx.
        for (int k = last_idx - 1; k >= 0; --k) {
            if (!fractals[k].fully_formed) continue;
            if (fractals[k].type > 0) {
                if (fractals[last_idx].price > fractals[k].price) return +1;
                return 0;
            }
        }
        return 0;
    } else {
        // Find previous DOWN fractal before last_idx.
        for (int k = last_idx - 1; k >= 0; --k) {
            if (!fractals[k].fully_formed) continue;
            if (fractals[k].type < 0) {
                if (fractals[last_idx].price < fractals[k].price) return -1;
                return 0;
            }
        }
        return 0;
    }
}
// Not-fully-formed fractal at the bar before the break.
// BUY  (dir > 0): potential DOWN fractal -> bar[break-1].low  < bar[break-2].low
// SELL (dir < 0): potential UP   fractal -> bar[break-1].high > bar[break-2].high
static bool find_not_fully_formed_stop(
    const std::vector<spartak::core::Bar>& bars,
    int break_idx,
    int dir,
    double& price)
{
    if (break_idx < 2) return false;
    const auto& L = bars[break_idx - 2];
    const auto& C = bars[break_idx - 1];
    if (dir > 0) {
        if (C.low < L.low) { price = C.low; return true; }
    } else {
        if (C.high > L.high) { price = C.high; return true; }
    }
    return false;
}
static void simulate_exit(
    const std::vector<spartak::core::Bar>& bars,
    int    dir,
    int    entry_idx,
    double entry_price,
    double sl_initial,
    double tp,
    double be_trigger,
    double point,
    int&   exit_idx,
    double& exit_price,
    std::string& reason)
{
    const int N = (int)bars.size();
    exit_idx = -1;
    exit_price = entry_price;
    reason = "eod";
    double sl = sl_initial;
    bool   be_moved = false;
    for (int i = entry_idx + 1; i < N; ++i) {
        if (dir > 0) {
            if (bars[i].high >= tp) {
                exit_idx = i; exit_price = tp; reason = "tp"; return;
            }
            if (!be_moved && bars[i].high >= entry_price + be_trigger) {
                sl = entry_price;
                be_moved = true;
            }
            if (bars[i].low <= sl) {
                exit_idx = i; exit_price = sl;
                reason = be_moved ? "be" : "sl";
                return;
            }
        } else {
            if (bars[i].low <= tp) {
                exit_idx = i; exit_price = tp; reason = "tp"; return;
            }
            if (!be_moved && bars[i].low <= entry_price - be_trigger) {
                sl = entry_price;
                be_moved = true;
            }
            if (bars[i].high >= sl) {
                exit_idx = i; exit_price = sl;
                reason = be_moved ? "be" : "sl";
                return;
            }
        }
    }
}
std::vector<CorridorEvent> scan_corridors(
    const std::vector<spartak::core::Bar>& bars,
    const std::vector<Fractal>&            fractals,
    const std::vector<DailyBar>&           daily,
    double point,
    double threshold_pts,
    bool   adr_filter,
    double adr_mult,
    double tp_mult)
{
    std::vector<CorridorEvent> raw;
    if (fractals.size() < 2) return raw;
    const int N = (int)bars.size();
    for (size_t k = 1; k < fractals.size(); ++k) {
        const Fractal& curr = fractals[k];
        if (!curr.fully_formed) continue;
        int dir = (curr.type > 0) ? +1 : -1;
        int stop_k = find_stop_fractal(fractals, (int)k, dir);
        if (stop_k < 0) continue;
        double start_line = curr.price;
        double stop_full  = fractals[stop_k].price;
        if (dir > 0 && stop_full >= start_line) continue;
        if (dir < 0 && stop_full <= start_line) continue;
        double height_full = std::fabs(start_line - stop_full);
        if (height_full <= 0.0) continue;
        int height_full_pts = (int)(height_full / point + 0.5);
        int adr5_pts_local = 0;
        if (adr_filter) {
            int dk = day_key_from_ms(bars[curr.bar_idx].timestamp);
            int di = find_day_idx(daily, dk);
            if (di < 5) continue;
            double adr5 = adr5_at(daily, di);
            if (adr5 <= 0.0) continue;
            adr5_pts_local = (int)(adr5 / point + 0.5);
            double max_h = adr_mult * adr5_pts_local;
            if ((double)height_full_pts > max_h) continue;
        }
        int prev_dir = find_previous_direction(fractals, (int)k);
        bool same_dir = (prev_dir == dir);
        for (int i = curr.bar_idx + 1; i < N; ++i) {
            double spread_pts = (double)bars[i].spread;
            if (spread_pts < 0) spread_pts = 0;
            double need = threshold_pts + spread_pts;
            bool triggered = false;
            if (dir > 0) triggered = (bars[i].high >= start_line + need * point);
            else         triggered = (bars[i].low  <= start_line - need * point);
            if (!triggered) continue;
            double stop_line  = stop_full;
            double height     = height_full;
            int    height_pts = height_full_pts;
            int    used_nff   = 0;
            if (same_dir) {
                double cand_price = 0.0;
                if (find_not_fully_formed_stop(bars, i, dir, cand_price)) {
                    bool ok_side = false;
                    if (dir > 0 && cand_price < start_line) ok_side = true;
                    if (dir < 0 && cand_price > start_line) ok_side = true;
                    if (ok_side) {
                        stop_line  = cand_price;
                        height     = std::fabs(start_line - cand_price);
                        height_pts = (int)(height / point + 0.5);
                        used_nff   = 1;
                    }
                }
            }
            CorridorEvent e;
            e.dir         = dir;
            e.break_idx   = i;
            e.entry_idx   = i;
            e.start_line  = start_line;
            e.stop_line   = stop_line;
            e.entry_price = (dir > 0)
                          ? start_line + need * point
                          : start_line - need * point;
            e.height      = height;
            e.height_pts  = height_pts;
            e.adr5_pts    = adr5_pts_local;
            e.exit_idx    = -1;
            e.exit_price  = 0.0;
            e.pnl_pts     = 0.0;
            e.year        = year_from_ms(bars[i].timestamp);
            e.adr_pct     = (adr5_pts_local > 0)
                          ? height_pts * 100 / adr5_pts_local
                          : 0;
            e.prev_dir_val = prev_dir;
            e.used_nff     = used_nff;
            raw.push_back(e);
            break;
        }
    }
    std::sort(raw.begin(), raw.end(),
              [](const CorridorEvent& a, const CorridorEvent& b) {
                  return a.entry_idx < b.entry_idx;
              });
    std::vector<CorridorEvent> unique_raw;
    unique_raw.reserve(raw.size());
    int last_idx = -1;
    for (const auto& e : raw) {
        if (e.entry_idx == last_idx) continue;
        unique_raw.push_back(e);
        last_idx = e.entry_idx;
    }
    std::vector<CorridorEvent> out;
    int last_exit_bar = -1;
    for (auto e : unique_raw) {
        if (e.entry_idx <= last_exit_bar) continue;
        double tp = (e.dir > 0)
            ? e.entry_price + e.height * tp_mult
            : e.entry_price - e.height * tp_mult;
        double sl = e.stop_line;
        double be_trigger = 2.1 * e.height;
        int exit_idx; double exit_price; std::string reason;
        simulate_exit(bars, e.dir, e.entry_idx, e.entry_price,
                      sl, tp, be_trigger, point,
                      exit_idx, exit_price, reason);
        e.exit_idx    = exit_idx;
        e.exit_price  = exit_price;
        e.exit_reason = reason;
        double dir_sign = (e.dir > 0) ? 1.0 : -1.0;
        e.pnl_pts = (exit_price - e.entry_price) * dir_sign / point;
        out.push_back(e);
        last_exit_bar = (exit_idx >= 0) ? exit_idx : N;
    }
    return out;
}
} // namespace st