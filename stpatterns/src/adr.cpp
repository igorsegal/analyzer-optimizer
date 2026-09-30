// =============================================================================
//  STPatterns :: st/adr.cpp
// =============================================================================
#include "st/adr.h"
#include <ctime>
#include <cstdio>
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

std::vector<DailyBar> build_daily(const std::vector<spartak::core::Bar>& bars)
{
    std::vector<DailyBar> out;
    if (bars.empty()) return out;

    DailyBar cur;
    cur.day_key   = day_key_from_ms(bars[0].timestamp);
    cur.high      = bars[0].high;
    cur.low       = bars[0].low;
    cur.bar_count = 1;

    for (size_t i = 1; i < bars.size(); ++i) {
        int k = day_key_from_ms(bars[i].timestamp);
        if (k == cur.day_key) {
            if (bars[i].high > cur.high) cur.high = bars[i].high;
            if (bars[i].low  < cur.low)  cur.low  = bars[i].low;
            ++cur.bar_count;
        } else {
            out.push_back(cur);
            cur.day_key   = k;
            cur.high      = bars[i].high;
            cur.low       = bars[i].low;
            cur.bar_count = 1;
        }
    }
    out.push_back(cur);
    return out;
}

double adr5_at(const std::vector<DailyBar>& daily, int day_idx)
{
    if (day_idx < 5) return 0.0;
    double sum = 0.0;
    for (int i = day_idx - 5; i < day_idx; ++i) {
        sum += (daily[i].high - daily[i].low);
    }
    return sum / 5.0;
}

} // namespace st