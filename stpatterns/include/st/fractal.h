// =============================================================================
//  STPatterns :: st/fractal.h
//  3-candle fractal detector.
// =============================================================================
#pragma once
#include "st/contracts.h"
#include "core/Types.h"
#include <vector>

namespace st {

// Scan all bars, return list of fractals in chronological order.
// Bar 0 and bar N-1 cannot be fractals (need both neighbors).
std::vector<Fractal> find_fractals(const std::vector<spartak::core::Bar>& bars);

// Fractal at bar i is "fully formed" if i+1 < bars.size() (right neighbor exists).
bool fractal_fully_formed(const std::vector<spartak::core::Bar>& bars, int i);

} // namespace st