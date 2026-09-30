// =============================================================================
//  STPatterns :: st/adr.h
//  Average Daily Range (5 days). Daily bars built from hourly.
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstdint>
#include <vector>

namespace st {

struct DailyBar {
    int64_t day_key;   // yyyymmdd as int
    double  high;
    double  low;
    int     bar_count;
};

// Build daily bars from H1 bars (group by UTC date).
std::vector<DailyBar> build_daily(const std::vector<spartak::core::Bar>& bars);

// ADR(5) ending at day_idx (uses days [day_idx-5 .. day_idx-1]).
// Returns 0.0 if not enough history.
double adr5_at(const std::vector<DailyBar>& daily, int day_idx);

} // namespace st