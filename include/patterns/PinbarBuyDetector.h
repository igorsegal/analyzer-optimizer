// =============================================================================
//  SPARTAK :: patterns/PinbarBuyDetector.h
//  Детектор бычьего пинбара (Hammer) для BUY-сигнала.
//
//  Условия:
//    - нижняя тень длинная        (lower_ratio >= min_lower_ratio)
//    - тело маленькое             (body_ratio  <= max_body_ratio)
//    - верхняя тень короткая      (upper_ratio <= max_upper_ratio)
//    - закрытие в верхней части   (close_position >= min_close_pos)
//
//  Использует CandleGeometryCalculator для базовых метрик.
//
//  Возвращает PinbarSignal с рекомендованным стопом (за low свечи
//  с буфером) и trigger_price (= close свечи, точка входа).
// =============================================================================
#pragma once
#include "core/Types.h"
#include "patterns/CandleGeometryCalculator.h"
#include <cstdint>
namespace spartak::patterns {
// -----------------------------------------------------------------------------
// Результат детекции пинбара.
// -----------------------------------------------------------------------------
struct PinbarSignal {
    bool   detected       = false;
    double confidence     = 0.0;    // 0..1 — «сила» сигнала
    double trigger_price  = 0.0;    // цена закрытия (вход)
    double suggested_stop = 0.0;    // за low с буфером
};
// -----------------------------------------------------------------------------
// Конфиг детектора.
// -----------------------------------------------------------------------------
struct PinbarConfig {
    double min_lower_ratio = 0.60;   // нижняя тень >= 60% range
    double max_body_ratio  = 0.30;   // тело <= 30% range
    double max_upper_ratio = 0.20;   // верхняя тень <= 20% range
    double min_close_pos   = 0.50;   // close в верхней половине
    int    stop_buffer_points = 10;  // отступ стопа за low
    double point             = 0.00001;
};
// -----------------------------------------------------------------------------
// PinbarBuyDetector — stateless.
// -----------------------------------------------------------------------------
class PinbarBuyDetector {
public:
    explicit PinbarBuyDetector(PinbarConfig cfg = {},
                               CandleGeometryConfig geom_cfg = {});
    // Проверка одной свечи.
    [[nodiscard]] PinbarSignal detect(const core::Bar& bar) const noexcept;
    const PinbarConfig&           config()     const noexcept { return cfg_; }
    const CandleGeometryCalculator& geometry() const noexcept { return geom_; }
private:
    PinbarConfig             cfg_;
    CandleGeometryCalculator geom_;
};
} // namespace spartak::patterns