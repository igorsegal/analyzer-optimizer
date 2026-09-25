// =============================================================================
//  SPARTAK :: context/OldLevelCleaner.h
//  Деактивирует устаревшие зоны (ОП/ОС).
//
//  Два критерия:
//    1. По возрасту: now - zone.formation_time > max_age_ms
//    2. По пробою: close ушёл за границу зоны на break_buffer_points
//       (пробитие зоны в любую сторону деактивирует уровень)
//
//  Деактивация = is_active = false. Зона не удаляется, а помечается —
//  чтобы downstream-модули могли её видеть и при необходимости реанимировать.
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstdint>
#include <vector>
namespace spartak::context {
// -----------------------------------------------------------------------------
// Конфиг очистки.
// -----------------------------------------------------------------------------
struct OldLevelConfig {
    int64_t max_age_ms           = 7LL * 86'400'000LL;  // 7 дней
    int     break_buffer_points  = 30;                  // отступ для «пробоя»
    double  point                = 0.00001;
};
// -----------------------------------------------------------------------------
// OldLevelCleaner — stateless.
// -----------------------------------------------------------------------------
class OldLevelCleaner {
public:
    explicit OldLevelCleaner(OldLevelConfig cfg = {});
    // Возвращает копию списка зон с обновлённым полем is_active.
    // Исходный вектор не изменяется.
    [[nodiscard]] std::vector<core::PriceZone>
    clean(const std::vector<core::PriceZone>& zones,
          int64_t now_ms,
          double  current_close) const;
    // Версия in-place (мутирует переданный вектор).
    void cleanInPlace(std::vector<core::PriceZone>& zones,
                      int64_t now_ms,
                      double  current_close) const;
    const OldLevelConfig& config() const noexcept { return cfg_; }
private:
    OldLevelConfig cfg_;
    // Одна зона — устарела?
    bool is_stale(const core::PriceZone& z,
                  int64_t now_ms,
                  double  current_close) const noexcept;
};
} // namespace spartak::context