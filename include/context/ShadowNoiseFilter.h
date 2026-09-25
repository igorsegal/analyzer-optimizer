// =============================================================================
//  SPARTAK :: context/ShadowNoiseFilter.h
//  Отсеивает «шумовые» экстремумы, которые не подтвердились откатом цены.
//
//  Логика (validated fractal):
//    Для High-экстремума с ценой P на индексе i:
//      следующие confirm_window баров не должны превысить P + tolerance_points.
//      Иначе — это шум (цена продолжила движение вверх, разворота не было).
//
//    Для Low-экстремума — симметрично: следующие бары не должны пробить
//    P - tolerance_points вниз.
//
//  Экстремумы у конца массива (где ещё не накопилось confirm_window баров)
//  отбрасываются — подтверждения по ним ещё нет.
// =============================================================================
#pragma once
#include "core/Types.h"
#include "context/FractalPointDetector.h"
#include <cstdint>
#include <cstddef>
#include <vector>
namespace spartak::context {
// -----------------------------------------------------------------------------
// Конфиг фильтра.
// -----------------------------------------------------------------------------
struct ShadowNoiseConfig {
    int     confirm_window    = 5;
    int     tolerance_points  = 2;
    double  point             = 0.00001;
};
// -----------------------------------------------------------------------------
// ShadowNoiseFilter — stateless.
// -----------------------------------------------------------------------------
class ShadowNoiseFilter {
public:
    explicit ShadowNoiseFilter(ShadowNoiseConfig cfg = {});
    [[nodiscard]] std::vector<Extremum>
    filter(const std::vector<core::Bar>& bars,
           const std::vector<Extremum>& extrema) const;
    const ShadowNoiseConfig& config() const noexcept { return cfg_; }
private:
    ShadowNoiseConfig cfg_;
};
} // namespace spartak::context