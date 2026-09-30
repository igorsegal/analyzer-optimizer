// =============================================================================
//  STPatterns :: st/corridor.cpp
// =============================================================================
#include "st/corridor.h"
#include <cmath>
#include <ctime>
#include <algorithm>
#include <climits>
namespace st {
static void utc_components(int64_t ms, int& year, int& mon, int& mday,
                           int& hour, int& wday)
{
    std::time_t secs = (std::time_t)(ms / 1000);
    std::tm tm{};
#if defined(_MSC_VER)
    gmtime_s(&tm, &secs);
#else
    gmtime_r(&secs, &tm);
#endif
    year = tm.tm_year + 1900;
    mon  = tm.tm_mon + 1;
    mday = tm.tm_mday;
    hour = tm.tm_hour;
    wday = tm.tm_wday;
}
static int day_key_from_ms(int64_t ms)
{
    int y, m, d, h, w;
    utc_components(ms, y, m, d, h, w);
    return y * 10000 + m * 100 + d;
}
static int year_from_ms(int64_t ms)
{
    int y, m, d, h, w;
    utc_components(ms, y, m, d, h, w);
    return y;
}
static bool in_session(int64_t ms)
{
    int y, m, d, h, w;
    utc_components(ms, y, m, d, h, w);
    if (h < 6 || h >= 19) return false;
    if (w == 5 && h >= 20) return false;
    if (w == 1 && h < 2)   return false;
    return true;
}
static int find_day_idx(const std::vector<DailyBar>& daily, int day_key)
{
    for (int i = 0; i < (int)daily.size(); ++i)
        if (daily[i].day_key == day_key) return i;
    return -1;
}
static int find_stop_fractal(const std::vector<Fractal>& fractals,
                             int curr_idx, int dir)
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
static int find_previous_direction(const std::vector<Fractal>& fractals,
                                   int curr_idx)
{
    int up1 = -1, up2 = -1;
    int dn1 = -1, dn2 = -1;
    for (int k = curr_idx - 1; k >= 0; --k) {
        if (!fractals[k].fully_formed) continue;
        if (fractals[k].type > 0) {
            if (up1 < 0) up1 = k;
            else if (up2 < 0) { up2 = k; break; }
        } else {
            if (dn1 < 0) dn1 = k;
            else if (dn2 < 0) { dn2 = k; break; }
        }
    }
    bool hh = false, ll = false;
    int  up_idx = -1, dn_idx = -1;
    if (up1 >= 0 && up2 >= 0 && fractals[up1].price > fractals[up2].price) {
        hh = true; up_idx = up1;
    }
    if (dn1 >= 0 && dn2 >= 0 && fractals[dn1].price < fractals[dn2].price) {
        ll = true; dn_idx = dn1;
    }
    if (hh && !ll) return +1;
    if (ll && !hh) return -1;
    if (hh && ll) return (up_idx > dn_idx) ? +1 : -1;
    return 0;
}
static bool find_not_fully_formed_stop(
    const std::vector<spartak::core::Bar>& bars,
    int break_idx, int dir, double& price)
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
// Simulate exit with Reverse Movement pattern.
// all_signals is the full sorted+deduped signal list.
// current_idx is index of the current trade in all_signals.
// Find first signal with opposite dir at index > current_idx
// with entry_idx before SL/TP. If exists and triggers before SL/TP
// -> exit at that signal's entry_price, reason = "reverse".
static void simulate_exit(
    const std::vector<spartak::core::Bar>& bars,
    const std::vector<CorridorEvent>&      all_signals,
    int    current_idx,
    int    dir,
    int    entry_idx,
    double entry_price,
    double start_line,
    double sl_initial,
    double height,
    double tp_mult,
    double be_mult,
    double point,
    int&   exit_idx,
    double& exit_price,
    std::string& reason)
{
    const int N = (int)bars.size();
    exit_idx = -1;
    exit_price = entry_price;
    reason = "eod";
    // Find first opposite signal after current_idx
    int reverse_bar = INT_MAX;
    double reverse_entry_px = 0.0;
    for (int k = current_idx + 1; k < (int)all_signals.size(); ++k) {
        if (all_signals[k].dir != dir) {
            reverse_bar = all_signals[k].entry_idx;
            reverse_entry_px = all_signals[k].entry_price;
            break;
        }
        // If we hit the same direction signal that's closer than any
        // opposite one, keep looking - not a reversal.
    }
    double tp = (dir > 0)
              ? start_line + height * tp_mult
              : start_line - height * tp_mult;
    double be_trigger = (dir > 0)
                      ? start_line + height * be_mult
                      : start_line - height * be_mult;
    double sl = sl_initial;
    bool   be_moved = false;
    for (int i = entry_idx + 1; i < N; ++i) {
        // 1. TP first (favorable)
        if (dir > 0) {
            if (bars[i].high >= tp) {
                exit_idx = i; exit_price = tp; reason = "tp"; return;
            }
        } else {
            if (bars[i].low <= tp) {
                exit_idx = i; exit_price = tp; reason = "tp"; return;
            }
        }
        // 2. BE update
        if (dir > 0) {
            if (!be_moved && bars[i].high >= be_trigger) {
                sl = entry_price;
                be_moved = true;
            }
        } else {
            if (!be_moved && bars[i].low <= be_trigger) {
                sl = entry_price;
                be_moved = true;
            }
        }
        // 3. SL/BE hit on this bar
        bool sl_hit = (dir > 0) ? (bars[i].low <= sl)
                                : (bars[i].high >= sl);
        if (sl_hit) {
            // If opposite signal triggered strictly BEFORE this bar
            // (i > reverse_bar) we would have exited earlier; but
            // if i == reverse_bar and both SL and reverse occur
            // on same bar, prefer SL (conservative).
            exit_idx = i;
            exit_price = sl;
            reason = be_moved ? "be" : "sl";
            return;
        }
        // 4. Reverse (opposite signal at this bar, no SL/TP hit yet)
        if (i == reverse_bar) {
            exit_idx = i;
            exit_price = reverse_entry_px;
            reason = "reverse";
            return;
        }
    }
}
std::vector<CorridorEvent> scan_corridors(
    const std::vector<spartak::core::Bar>& bars,
    const std::vector<Fractal>&            fractals,
    const std::vector<DailyBar>&           daily,
    double point,
    bool   adr_filter,
    double adr_mult,
    double tp_mult)
{
    std::vector<CorridorEvent> raw;
    if (fractals.size() < 2) return raw;
    if (point <= 0.0) return raw;
    const int N = (int)bars.size();
    const double threshold_frac = 0.12;
    const double min_rr = 1.5;
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
        double adr5_price  = 0.0;
        if (adr_filter) {
            int dk = day_key_from_ms(bars[curr.bar_idx].timestamp);
            int di = find_day_idx(daily, dk);
            if (di < 5) continue;
            adr5_price = adr5_at(daily, di);
            if (adr5_price <= 0.0) continue;
            adr5_pts_local = (int)(adr5_price / point + 0.5);
            if (adr5_pts_local <= 0) continue;
            double max_h = adr_mult * adr5_pts_local;
            if ((double)height_full_pts > max_h) continue;
        } else {
            adr5_price = height_full / 0.25;
            adr5_pts_local = (int)(adr5_price / point + 0.5);
        }
        double need_pts = threshold_frac * adr5_pts_local;
        if (need_pts < 1.0) need_pts = 1.0;
        int prev_dir = find_previous_direction(fractals, (int)k);
        bool same_dir = (prev_dir == dir);
        for (int i = curr.bar_idx + 1; i < N; ++i) {
            if (!in_session(bars[i].timestamp)) continue;
            double spread_pts = (double)bars[i].spread;
            if (spread_pts < 0) spread_pts = 0;
            double need = need_pts + spread_pts;
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
            double entry_px = (dir > 0)
                            ? start_line + need * point
                            : start_line - need * point;
            double tp_px    = (dir > 0)
                            ? start_line + height * tp_mult
                            : start_line - height * tp_mult;
            double dist_tp  = (dir > 0) ? (tp_px - entry_px)
                                        : (entry_px - tp_px);
            double dist_sl  = (dir > 0) ? (entry_px - stop_line)
                                        : (stop_line - entry_px);
            if (dist_sl <= 0.0) continue;
            double rr_val = dist_tp / dist_sl;
            if (rr_val < min_rr) continue;
            CorridorEvent e;
            e.dir         = dir;
            e.break_idx   = i;
            e.entry_idx   = i;
            e.start_line  = start_line;
            e.stop_line   = stop_line;
            e.entry_price = entry_px;
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
    for (size_t si = 0; si < unique_raw.size(); ++si) {
        auto e = unique_raw[si];
        if (e.entry_idx <= last_exit_bar) continue;
        int exit_idx; double exit_price; std::string reason;
        simulate_exit(bars, unique_raw, (int)si,
                      e.dir, e.entry_idx, e.entry_price,
                      e.start_line, e.stop_line, e.height,
                      tp_mult, 2.1, point,
                      exit_idx, exit_price, reason);
        e.exit_idx    = exit_idx;
        e.exit_price  = exit_price;
        e.exit_reason = reason;
        double dir_sign = (e.dir > 0) ? 1.0 : -1.0;
        e.pnl_pts = (exit_price - e.entry_price) * dir_sign / point;
        out.push_back(e);
        if (reason == "reverse" && exit_idx >= 0)
            last_exit_bar = exit_idx - 1;   // include the reversing signal
        else
            last_exit_bar = (exit_idx >= 0) ? exit_idx : N;
    }
    return out;
}
} // namespace st