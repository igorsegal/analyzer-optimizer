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

static int find_day_idx(const std::vector<DailyBar>& daily, int day_key)
{
    for (int i = 0; i < (int)daily.size(); ++i)
        if (daily[i].day_key == day_key) return i;
    return -1;
}

// Simulate: from entry_idx + 1 forward, find first SL or TP hit.
// Returns exit bar index. Sets exit_price, reason.
// If no exit before end of bars - exit_idx = -1, reason = "eod".
static void simulate_exit(
    const std::vector<spartak::core::Bar>& bars,
    int    dir,
    int    entry_idx,
    double entry_price,
    double sl,
    double tp,
    double point,
    int&   exit_idx,
    double& exit_price,
    std::string& reason)
{
    const int N = (int)bars.size();
    exit_idx = -1;
    exit_price = entry_price;
    reason = "eod";

    for (int i = entry_idx + 1; i < N; ++i) {
        if (dir > 0) {
            // BUY: SL below entry (stop_line), TP above (entry + tp_mult*H)
            if (bars[i].low <= sl) {
                exit_idx = i;
                exit_price = sl;
                reason = "sl";
                return;
            }
            if (bars[i].high >= tp) {
                exit_idx = i;
                exit_price = tp;
                reason = "tp";
                return;
            }
        } else {
            // SELL: SL above entry, TP below
            if (bars[i].high >= sl) {
                exit_idx = i;
                exit_price = sl;
                reason = "sl";
                return;
            }
            if (bars[i].low <= tp) {
                exit_idx = i;
                exit_price = tp;
                reason = "tp";
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

    // --- Pass 1: generate all raw signals ---
    for (size_t k = 1; k < fractals.size(); ++k) {
        const Fractal& prev = fractals[k - 1];
        const Fractal& curr = fractals[k];

        if (prev.type == curr.type) continue;
        if (!prev.fully_formed || !curr.fully_formed) continue;

        int dir = (curr.type > 0) ? +1 : -1;
        double start_line = curr.price;
        double stop_line  = prev.price;

        double height = std::fabs(start_line - stop_line);
        if (height <= 0.0) continue;
        int height_pts = (int)(height / point + 0.5);

        if (adr_filter) {
            int dk = day_key_from_ms(bars[curr.bar_idx].timestamp);
            int di = find_day_idx(daily, dk);
            if (di < 5) continue;
            double adr5 = adr5_at(daily, di);
            if (adr5 <= 0.0) continue;
            double adr5_pts = adr5 / point;
            double max_h = adr_mult * adr5_pts;
            if ((double)height_pts > max_h) continue;
        }

        for (int i = curr.bar_idx + 1; i < N; ++i) {
            double spread_pts = (double)bars[i].spread;
            if (spread_pts < 0) spread_pts = 0;
            double need = threshold_pts + spread_pts;

            if (dir > 0) {
                if (bars[i].high >= start_line + need * point) {
                    CorridorEvent e;
                    e.dir = +1; e.break_idx = i; e.entry_idx = i;
                    e.start_line = start_line; e.stop_line = stop_line;
                    e.entry_price = start_line + need * point;
                    e.height = height; e.height_pts = height_pts;
                    e.adr5_pts = 0;
                    e.exit_idx = -1; e.exit_price = 0; e.pnl_pts = 0;
                    raw.push_back(e);
                    break;
                }
            } else {
                if (bars[i].low <= start_line - need * point) {
                    CorridorEvent e;
                    e.dir = -1; e.break_idx = i; e.entry_idx = i;
                    e.start_line = start_line; e.stop_line = stop_line;
                    e.entry_price = start_line - need * point;
                    e.height = height; e.height_pts = height_pts;
                    e.adr5_pts = 0;
                    e.exit_idx = -1; e.exit_price = 0; e.pnl_pts = 0;
                    raw.push_back(e);
                    break;
                }
            }
        }
    }

    // --- Sort by entry_idx, dedup per bar ---
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

    // --- Pass 2: non-overlap filter + simulate exit ---
    std::vector<CorridorEvent> out;
    int last_exit_bar = -1;

    for (auto e : unique_raw) {
        if (e.entry_idx <= last_exit_bar) continue;

        double tp = (e.dir > 0)
            ? e.entry_price + e.height * tp_mult
            : e.entry_price - e.height * tp_mult;
        double sl = e.stop_line;

        int exit_idx; double exit_price; std::string reason;
        simulate_exit(bars, e.dir, e.entry_idx, e.entry_price,
                      sl, tp, point, exit_idx, exit_price, reason);

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