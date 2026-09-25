#include "validation/SessionTimeFilter.h"
#include <stdexcept>
namespace spartak::validation {
SessionTimeFilter::SessionTimeFilter(SessionConfig cfg)
    : cfg_(cfg) {
    if (cfg_.begin_hour_utc < 0 || cfg_.begin_hour_utc > 24)
        throw std::invalid_argument("SessionConfig::begin_hour_utc must be in [0,24]");
    if (cfg_.end_hour_utc < 0 || cfg_.end_hour_utc > 24)
        throw std::invalid_argument("SessionConfig::end_hour_utc must be in [0,24]");
}
// -----------------------------------------------------------------------------
// hour_utc — часы UTC из Unix ms.
// -----------------------------------------------------------------------------
int SessionTimeFilter::hour_utc(int64_t timestamp_ms) noexcept {
    int64_t sec = timestamp_ms / 1000;
    int64_t day_sec = sec % 86400;
    if (day_sec < 0) day_sec += 86400;
    return static_cast<int>(day_sec / 3600);
}
// -----------------------------------------------------------------------------
// pass — попадает ли timestamp в разрешённое окно.
//
// Логика:
//   if (!enabled) -> true (фильтр выключен);
//   begin == end  -> окно пустое, но трактуем как 24h (никогда не блокируем);
//   begin <  end  -> [begin, end);
//   begin >  end  -> «через полночь»: [begin, 24) U [0, end).
// -----------------------------------------------------------------------------
bool SessionTimeFilter::pass(int64_t timestamp_ms) const noexcept {
    if (!cfg_.enabled) return true;
    const int h = hour_utc(timestamp_ms);
    const int b = cfg_.begin_hour_utc;
    const int e = cfg_.end_hour_utc;
    if (b == e) return true;   // пустое окно = разрешено всё
    if (b < e) {
        return h >= b && h < e;
    }
    // Через полночь: [b, 24) U [0, e)
    return h >= b || h < e;
}
bool SessionTimeFilter::pass(const core::Bar& bar) const noexcept {
    return pass(bar.timestamp);
}
} // namespace spartak::validation