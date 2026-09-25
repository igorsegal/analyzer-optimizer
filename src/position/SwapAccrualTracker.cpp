#include "position/SwapAccrualTracker.h"
#include <stdexcept>
namespace spartak::position {
SwapAccrualTracker::SwapAccrualTracker(SwapConfig cfg)
    : cfg_(cfg) {
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("SwapConfig::point must be > 0");
    if (cfg_.contract_size <= 0.0)
        throw std::invalid_argument("SwapConfig::contract_size must be > 0");
}
int64_t SwapAccrualTracker::day_of(int64_t timestamp_ms) noexcept {
    // floor для отрицательных тоже корректен (на практике время > 1970)
    if (timestamp_ms >= 0) return timestamp_ms / core::numeric::MS_PER_DAY;
    // Отрицательные времена не встречаются в XFBAR, но защитимся
    const int64_t d = core::numeric::MS_PER_DAY;
    return (timestamp_ms - d + 1) / d;
}
SwapAccrual SwapAccrualTracker::compute(core::OrderSide side,
                                        double volume,
                                        int64_t last_swap_day,
                                        int64_t now_ms) const noexcept
{
    SwapAccrual r;
    const int64_t today = day_of(now_ms);
    if (last_swap_day < 0) {
        // Первичная инициализация: фиксируем день открытия
        r.accrued   = false;
        r.nights    = 0;
        r.money     = 0.0;
        r.new_day   = today;
        return r;
    }
    if (today <= last_swap_day) {
        // Полночь ещё не пересекали
        r.accrued   = false;
        r.nights    = 0;
        r.money     = 0.0;
        r.new_day   = last_swap_day;
        return r;
    }
    const int nights = static_cast<int>(today - last_swap_day);
    const double pts = (side == core::OrderSide::Buy)
                     ? cfg_.swap_long_points
                     : cfg_.swap_short_points;
    // money = pts * point * contract_size * volume * nights
    const double per_night = pts * cfg_.point * cfg_.contract_size * volume;
    r.accrued  = true;
    r.nights   = nights;
    r.money    = per_night * static_cast<double>(nights);
    r.new_day  = today;
    return r;
}
} // namespace spartak::position