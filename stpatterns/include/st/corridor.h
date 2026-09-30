// =============================================================================
//  STPatterns :: st/corridor.h
// =============================================================================
#pragma once
#include "st/contracts.h"
#include "st/fractal.h"
#include "st/adr.h"
#include "core/Types.h"
#include <vector>

namespace st {

struct CorridorEvent {
    int     dir;
    int     break_idx;
    int     entry_idx;
    double  start_line;
    double  stop_line;
    double  entry_price;
    double  height;
    int     height_pts;
    int     adr5_pts;

    // Simulated position (for non-overlap filter).
    int     exit_idx;
    double  exit_price;
    double  pnl_pts;      // signed, in points
    std::string exit_reason;   // "sl" | "tp" | "eod"
};

std::vector<CorridorEvent> scan_corridors(
    const std::vector<spartak::core::Bar>& bars,
    const std::vector<Fractal>&            fractals,
    const std::vector<DailyBar>&           daily,
    double point,
    double threshold_pts,
    bool   adr_filter,
    double adr_mult,
    double tp_mult);

} // namespace st