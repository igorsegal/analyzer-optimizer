#include "position/VolumeSplitter50.h"
#include <cmath>
#include <stdexcept>
namespace spartak::position {
VolumeSplitter50::VolumeSplitter50(VolumeSplitterConfig cfg) : cfg_(cfg) {
    if (cfg_.min_lot <= 0.0)   throw std::invalid_argument("min_lot");
    if (cfg_.lot_step <= 0.0)  throw std::invalid_argument("lot_step");
    double s = 0.0;
    for (auto p : cfg_.partial_pcts) {
        if (p <= 0.0) throw std::invalid_argument("partial_pcts");
        s += p;
    }
    if (std::fabs(s - 1.0) > 1e-6) throw std::invalid_argument("partial_pcts must sum to 1.0");
}
SplitResult VolumeSplitter50::split(double initial, double remaining, int level) const noexcept {
    SplitResult r;
    if (initial <= 0.0 || remaining <= 0.0) return r;
    if (level < 0 || level > 2) return r;
    const double pct = cfg_.partial_pcts[level];
    const double raw = initial * pct;
    auto floor_step = [&](double v) {
        if (v <= 0.0) return 0.0;
        double out = std::floor((v + 1e-9) / cfg_.lot_step) * cfg_.lot_step;
        return std::round(out * 1e8) / 1e8;
    };
    double close_lot = floor_step(raw);
    double remain    = floor_step(remaining - close_lot);
    const bool close_bad  = close_lot + 1e-9 < cfg_.min_lot;
    const bool remain_bad = remain + 1e-9 < cfg_.min_lot;
    if (close_bad || remain_bad) {
        r.ok = true; r.full_close = true;
        r.close_lot = remaining; r.remain_lot = 0.0;
        return r;
    }
    r.ok = true; r.close_lot = close_lot; r.remain_lot = remain;
    return r;
}
} // namespace spartak::position