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

        int start_scan = curr.bar_idx + 1;

        for (int i = start_scan; i < N; ++i) {
            double spread_pts = (double)bars[i].spread;
            if (spread_pts < 0) spread_pts = 0;
            double need = threshold_pts + spread_pts;

            if (dir > 0) {
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
                    break;
                }
            }
        }
    }

    // Sort by entry bar index.
    std::sort(out.begin(), out.end(),
              [](const CorridorEvent& a, const CorridorEvent& b) {
                  return a.entry_idx < b.entry_idx;
              });

    // Dedup: one signal per bar, keep first occurrence.
    std::vector<CorridorEvent> unique_out;
    unique_out.reserve(out.size());
    int last_idx = -1;
    for (const auto& e : out) {
        if (e.entry_idx == last_idx) continue;
        unique_out.push_back(e);
        last_idx = e.entry_idx;
    }
    return unique_out;
}

} // namespace st