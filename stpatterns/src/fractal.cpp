// =============================================================================
//  STPatterns :: st/fractal.cpp
// =============================================================================
#include "st/fractal.h"

namespace st {

bool fractal_fully_formed(const std::vector<spartak::core::Bar>& bars, int i)
{
    return (i > 0) && (i + 1 < (int)bars.size());
}

std::vector<Fractal> find_fractals(const std::vector<spartak::core::Bar>& bars)
{
    std::vector<Fractal> out;
    const int n = (int)bars.size();
    if (n < 3) return out;

    for (int i = 1; i < n - 1; ++i) {
        const auto& L = bars[i - 1];
        const auto& C = bars[i];
        const auto& R = bars[i + 1];

        // Upper fractal: center.high >= both neighbors,
        //                and strictly > at least one side.
        if (C.high >= L.high && C.high >= R.high
            && (C.high > L.high || C.high > R.high)) {
            Fractal f;
            f.type          = 1;
            f.bar_idx       = i;
            f.price         = C.high;
            f.fully_formed  = true;   // i+1 exists by loop bound
            out.push_back(f);
        }

        // Lower fractal: center.low <= both neighbors,
        //                and strictly < at least one side.
        if (C.low <= L.low && C.low <= R.low
            && (C.low < L.low || C.low < R.low)) {
            Fractal f;
            f.type          = -1;
            f.bar_idx       = i;
            f.price         = C.low;
            f.fully_formed  = true;
            out.push_back(f);
        }
    }
    return out;
}

} // namespace st