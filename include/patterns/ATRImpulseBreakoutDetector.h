// =============================================================================
//  SPARTAK :: patterns/ATRImpulseBreakoutDetector.h
//  Детектор импульсного прорыва уровня (ATR-filtered).
//
//  Логика (BUY):
//    - свеча бычья;
//    - тело большое:      body_ratio >= min_body_ratio;
//    - close в верхней части: close_position >= min_close_pos;
//    - свеча крупнее среднего: range >= atr_multiplier * ATR(period);
//    - close > level (прорыв уровня).
//
//  Для SELL — зеркально.
//
//  Использует CandleGeometryCalculator для метрик свечи и ATR.
// =============================================================================
#pragma once
#include "core/Types.h"
#include "patterns/CandleGeometryCalculator.h"
#include <cstddef>
#include <vector>
namespace spartak::patterns {
// -----------------------------------------------------------------------------
// Результат детекции.
// -----------------------------------------------------------------------------
struct ImpulseSignal {
    bool   detected       = false;
    bool   is_bullish     = false;   // true = BUY-импульс, false = SELL-импульс
    double confidence     = 0.0;     // 0..1
    double trigger_price  = 0.0;     // = close бара
    double suggested_stop = 0.0;     // за low/high бара с буфером
    double level          = 0.0;     // пробитый уровень
    double atr            = 0.0;     // ATR на момент анализа (для диагностики)
};
// -----------------------------------------------------------------------------
// Конфиг детектора.
// -----------------------------------------------------------------------------
struct ImpulseConfig {
    double min_body_ratio     = 0.60;   // body >= 60% range
    double min_close_pos      = 0.75;   // для BUY; для SELL — <= 1-min_close_pos
    double atr_multiplier     = 1.5;    // range >= 1.5 * ATR
    int    atr_period         = 14;
    int    stop_buffer_points = 20;
    double point              = 0.00001;
};
// -----------------------------------------------------------------------------
// ATRImpulseBreakoutDetector — stateless.
// -----------------------------------------------------------------------------
class ATRImpulseBreakoutDetector {
public:
    explicit ATRImpulseBreakoutDetector(ImpulseConfig cfg = {},
                                        CandleGeometryConfig geom_cfg = {});
    // bars  — история с текущим баром последним
    // level — пробиваемый уровень (обычно price_level зоны)
    [[nodiscard]] ImpulseSignal detect(const std::vector<core::Bar>& bars,
                                       double level) const noexcept;
    const ImpulseConfig&            config()   const noexcept { return cfg_; }
    const CandleGeometryCalculator& geometry() const noexcept { return geom_; }
private:
    ImpulseConfig            cfg_;
    CandleGeometryCalculator geom_;
};
} // namespace spartak::patterns