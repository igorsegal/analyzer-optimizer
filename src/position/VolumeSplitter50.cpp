#include "position/VolumeSplitter50.h"
#include <cmath>
#include <stdexcept>
namespace spartak::position {
VolumeSplitter50::VolumeSplitter50(VolumeSplitterConfig cfg)
    : cfg_(cfg) {
    if (cfg_.partial_pct <= 0.0 || cfg_.partial_pct >= 1.0)
        throw std::invalid_argument("VolumeSplitterConfig::partial_pct must be in (0,1)");
    if (cfg_.min_lot <= 0.0)
        throw std::invalid_argument("VolumeSplitterConfig::min_lot must be > 0");
    if (cfg_.lot_step <= 0.0)
        throw std::invalid_argument("VolumeSplitterConfig::lot_step must be > 0");
}
// -----------------------------------------------------------------------------
// split — разбить объём.
//
// Алгоритм:
//   1. raw_close  = initial * partial_pct
//   2. close_lot  = floor(raw_close / step) * step
//   3. remain_lot = floor((remaining - close_lot) / step) * step
//   4. Если close_lot < min_lot или remain_lot < min_lot
//         -> full_close = true, close_lot = remaining, remain_lot = 0
//   5. Иначе ok=true, возвращаем обе части.
// -----------------------------------------------------------------------------
SplitResult VolumeSplitter50::split(double initial, double remaining) const noexcept {
    SplitResult r;
    if (initial <= 0.0 || remaining <= 0.0) return r;
    const double raw_close = initial * cfg_.partial_pct;
    auto floor_to_step = [&](double v) {
        if (v <= 0.0) return 0.0;
        const double steps = std::floor((v + 1e-9) / cfg_.lot_step);
        double out = steps * cfg_.lot_step;
        out = std::round(out * 1e8) / 1e8;
        return out;
    };
    double close_lot = floor_to_step(raw_close);
    // Если остаток после закрытия меньше min_lot — закрываем всё
    const double remain_raw = remaining - close_lot;
    double remain_lot = floor_to_step(remain_raw);
    const bool close_bad  = (close_lot + 1e-9 < cfg_.min_lot);
    const bool remain_bad = (remain_lot + 1e-9 < cfg_.min_lot);
    if (close_bad || remain_bad) {
        r.ok          = true;
        r.full_close  = true;
        r.close_lot   = remain_lot;   // внимательно: закрываем весь остаток
        r.remain_lot  = 0.0;
        // В защиту от остатков, занулим и принудительно поставим весь remaining
        r.close_lot   = remaining;
        return r;
    }
    r.ok          = true;
    r.full_close  = false;
    r.close_lot   = close_lot;
    r.remain_lot  = remain_lot;
    return r;
}
} // namespace spartak::position