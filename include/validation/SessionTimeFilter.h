// =============================================================================
//  SPARTAK :: validation/SessionTimeFilter.h
//  Торговый фильтр по времени (час UTC).
//
//  Проверяет, что текущий бар попадает в разрешённое окно.
//  Использует только часы UTC — так не зависим от DST и локали.
//
//  Позволяет как правило [begin, end), так и «через полночь»
//  (например, [22, 6) для азиатской сессии).
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstdint>
namespace spartak::validation {
// -----------------------------------------------------------------------------
// Конфиг.
// -----------------------------------------------------------------------------
struct SessionConfig {
    int  begin_hour_utc = 0;    // включительно
    int  end_hour_utc   = 24;   // исключая (24 = до конца дня)
    bool enabled        = true; // если false — фильтр выключен
};
// -----------------------------------------------------------------------------
// SessionTimeFilter — stateless.
// -----------------------------------------------------------------------------
class SessionTimeFilter {
public:
    explicit SessionTimeFilter(SessionConfig cfg = {});
    // Проверка по timestamp бара (Unix ms).
    [[nodiscard]] bool pass(int64_t timestamp_ms) const noexcept;
    // Проверка по бару целиком.
    [[nodiscard]] bool pass(const core::Bar& bar) const noexcept;
    // Извлечь час UTC из timestamp (0..23).
    [[nodiscard]] static int hour_utc(int64_t timestamp_ms) noexcept;
    const SessionConfig& config() const noexcept { return cfg_; }
private:
    SessionConfig cfg_;
};
} // namespace spartak::validation