// =============================================================================
//  STPatterns :: st/corridor.h
//  Fractal corridor + entry signal.
// =============================================================================
#pragma once
#include "st/contracts.h"
#include "st/fractal.h"
#include "st/adr.h"
#include "core/Types.h"
#include <vector>

namespace st {

struct CorridorEvent {
    int     dir;               // +1 buy (break up), -1 sell (break down)
    int     break_idx;         // bar index where start_line broken
    int     entry_idx;         // bar index where threshold reached
    double  start_line;        // broken fractal price
    double  stop_line;         // opposite fractal price
    double  entry_price;       // start_line +/- threshold
    double  height;            // |start - stop|
    int     height_pts;
    int     adr5_pts;
};

// Scan bars, find corridors and entry signals.
// fractals must be sorted chronologically.
// threshold_pts - min distance past start_line for entry
// adr_filter   - if true, apply height <= 0.5 * ADR(5) rule
// adr_mult     - multiplier (0.5 default)
std::vector<CorridorEvent> scan_corridors(
    const std::vector<spartak::core::Bar>& bars,
    const std::vector<Fractal>&            fractals,
    const std::vector<DailyBar>&           daily,
    double point,
    double threshold_pts,
    bool   adr_filter,
    double adr_mult);

} // namespace st