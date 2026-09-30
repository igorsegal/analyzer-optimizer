// =============================================================================
//  STPatterns :: st/corridor.cpp
// =============================================================================
#include "st/corridor.h"
#include <cmath>
#include <ctime>

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

std::vector<CorridorEvent> scan_corridors(
    const std::vector<spartak::core::Bar>& bars,
    const std::vector<Fractal>&            fractals,
    const std::vector<DailyBar>&           daily,
    double point,
    double threshold_pts,
    bool   adr_filter,
    double adr_mult)
{
    std::vector<CorridorEvent> out;
    if (fractals.size() < 2) return out;
    const int N = (int)bars.size();

    // Track bars already consumed by a signal to avoid duplicates.
    int last_break_bar = -1;

    for (size_t k = 1; k < fractals.size(); ++k) {
        const Fractal& prev = fractals[k - 1];
        const Fractal& curr = fractals[k];

        // Corridor needs OPPOSITE fractals.
        if (prev.type == curr.type) continue;

        // Only fully-formed fractals (both neighbors existed).
        if (!prev.fully_formed || !curr.fully_formed) continue;

        // Determine corridor direction: which fractal is "broken".
        // The more recent fractal (curr) is the potential break level.
        // Break direction: if curr is a DOWN fractal, price must close BELOW
        // its low -> SELL setup. If curr is UP, BUY setup on break above.
        int dir = (curr.type > 0) ? +1 : -1;
        double start_line = curr.price;
        double stop_line  = prev.price;

        double height = std::fabs(start_line - stop_line);
        if (height <= 0.0) continue;
        int height_pts = (int)(height / point + 0.5);

        // ADR filter
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

        // Look for break after curr.bar_idx.
        // Skip bars already used.
        int start_scan = curr.bar_idx + 1;
        if (start_scan <= last_break_bar) start_scan = last_break_bar + 1;

        for (int i = start_scan; i < N; ++i) {
            if (bars[i].spread < 0) continue;
            double spread_pts = (double)bars[i].spread;
            double need = threshold_pts + spread_pts;

            if (dir > 0) {
                // BUY: bar.low touched start_line and closed above
                if (bars[i].high >= start_line + need * point) {
                    CorridorEvent e;
                    e.dir         = +1;
                    e.break_idx   = i;
                    e.entry_idx   = i;
                    e.start_line  = start_line;
                    e.stop_line   = stop_line;
                    e.entry_price = start_line + need * point;
                    e.height      = height;
                    e.height_pts  = height_pts;
                    e.adr5_pts    = 0;
                    out.push_back(e);
                    last_break_bar = i;
                    break;
                }
            } else {
                if (bars[i].low <= start_line - need * point) {
                    CorridorEvent e;
                    e.dir         = -1;
                    e.break_idx   = i;
                    e.entry_idx   = i;
                    e.start_line  = start_line;
                    e.stop_line   = stop_line;
                    e.entry_price = start_line - need * point;
                    e.height      = height;
                    e.height_pts  = height_pts;
                    e.adr5_pts    = 0;
                    out.push_back(e);
                    last_break_bar = i;
                    break;
                }
            }
        }
    }
    return out;
}

} // namespace st