#include "context/OldLevelCleaner.h"
#include <cmath>
#include <stdexcept>
namespace spartak::context {
// -----------------------------------------------------------------------------
// Конструктор с валидацией.
// -----------------------------------------------------------------------------
OldLevelCleaner::OldLevelCleaner(OldLevelConfig cfg)
    : cfg_(cfg) {
    if (cfg_.max_age_ms <= 0)
        throw std::invalid_argument("OldLevelCleaner: max_age_ms must be > 0");
    if (cfg_.break_buffer_points < 0)
        throw std::invalid_argument("OldLevelCleaner: break_buffer_points must be >= 0");
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("OldLevelCleaner: point must be > 0");
}
// -----------------------------------------------------------------------------
// is_stale — приватная проверка одной зоны.
//
// Условия «устаревания»:
//   A) Возраст: (now_ms - formation_time) > max_age_ms
//   B) Пробой: close выше zone_top + buffer ИЛИ close ниже zone_bottom - buffer
//
// Пробой в ЛЮБУЮ сторону деактивирует уровень — потому что методика ТАП
// считает «съеденный» уровень отработанным.
// -----------------------------------------------------------------------------
bool OldLevelCleaner::is_stale(const core::PriceZone& z,
                               int64_t now_ms,
                               double  current_close) const noexcept
{
    // A) Возраст
    if (z.formation_time > 0 && now_ms > z.formation_time) {
        const int64_t age = now_ms - z.formation_time;
        if (age > cfg_.max_age_ms) return true;
    }
    // B) Пробой (в любую сторону)
    const double buffer = static_cast<double>(cfg_.break_buffer_points) * cfg_.point;
    if (current_close > z.zone_top + buffer)    return true;
    if (current_close < z.zone_bottom - buffer) return true;
    return false;
}
// -----------------------------------------------------------------------------
// clean — иммутабельная версия.
// -----------------------------------------------------------------------------
std::vector<core::PriceZone>
OldLevelCleaner::clean(const std::vector<core::PriceZone>& zones,
                       int64_t now_ms,
                       double  current_close) const
{
    std::vector<core::PriceZone> out = zones;
    cleanInPlace(out, now_ms, current_close);
    return out;
}
// -----------------------------------------------------------------------------
// cleanInPlace — мутирует переданный вектор.
// -----------------------------------------------------------------------------
void OldLevelCleaner::cleanInPlace(std::vector<core::PriceZone>& zones,
                                   int64_t now_ms,
                                   double  current_close) const
{
    for (auto& z : zones) {
        if (is_stale(z, now_ms, current_close)) {
            z.is_active = false;
        }
    }
}
} // namespace spartak::context