#include "context/ShadowNoiseFilter.h"
#include <stdexcept>
namespace spartak::context {
// -----------------------------------------------------------------------------
// Конструктор с валидацией.
// -----------------------------------------------------------------------------
ShadowNoiseFilter::ShadowNoiseFilter(ShadowNoiseConfig cfg)
    : cfg_(cfg) {
    if (cfg_.confirm_window < 1)
        throw std::invalid_argument("ShadowNoiseFilter: confirm_window must be >= 1");
    if (cfg_.tolerance_points < 0)
        throw std::invalid_argument("ShadowNoiseFilter: tolerance_points must be >= 0");
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("ShadowNoiseFilter: point must be > 0");
}
// -----------------------------------------------------------------------------
// filter — основная логика.
//
// Для каждого экстремума:
//   1. Если index + confirm_window выходит за пределы bars — экстремум
//      слишком близко к концу, подтверждения нет → отбрасываем.
//   2. Для High: проверяем следующий confirm_window баров:
//        если any bar.high > ext.price + tolerance → шум.
//   3. Для Low: симметрично:
//        если any bar.low < ext.price - tolerance → шум.
//
// tolerance = tolerance_points * point (в цене).
// -----------------------------------------------------------------------------
std::vector<Extremum>
ShadowNoiseFilter::filter(const std::vector<core::Bar>& bars,
                          const std::vector<Extremum>& extrema) const
{
    std::vector<Extremum> out;
    out.reserve(extrema.size());
    if (bars.empty() || extrema.empty()) return out;
    const double tolerance = static_cast<double>(cfg_.tolerance_points) * cfg_.point;
    const std::size_t n = bars.size();
    for (const auto& e : extrema) {
        // Границы массива
        const std::size_t start = e.index + 1;
        const std::size_t end   = e.index + static_cast<std::size_t>(cfg_.confirm_window) + 1;
        // Недостаточно баров для подтверждения — пропускаем
        if (end > n) continue;
        bool is_noise = false;
        if (e.kind == Extremum::Kind::High) {
            const double limit = e.price + tolerance;
            for (std::size_t i = start; i < end; ++i) {
                if (bars[i].high > limit) {
                    is_noise = true;
                    break;
                }
            }
        } else {  // Low
            const double limit = e.price - tolerance;
            for (std::size_t i = start; i < end; ++i) {
                if (bars[i].low < limit) {
                    is_noise = true;
                    break;
                }
            }
        }
        if (!is_noise) out.push_back(e);
    }
    return out;
}
} // namespace spartak::context