// =============================================================================
//  SPARTAK :: patterns/PinbarSellDetector.h
//  Детектор медвежьего пинбара (Shooting Star) для SELL-сигнала.
//
//  Условия (зеркальны BUY):
//    - верхняя тень длинная        (upper_ratio >= min_upper_ratio)
//    - тело маленькое              (body_ratio  <= max_body_ratio)
//    - нижняя тень короткая        (lower_ratio <= max_lower_ratio)
//    - закрытие в нижней части     (close_position <= max_close_pos)
// =============================================================================
#pragma once
#include "core/Types.h"
#include "patterns/CandleGeometryCalculator.h"
namespace spartak::patterns {
struct PinbarSignal;   // forward из PinbarBuyDetector.h
// -----------------------------------------------------------------------------
// PinbarSellSignal — отдельная структура (совпадает по форме с PinbarSignal).
// -----------------------------------------------------------------------------
struct PinbarSellSignal {
    bool   detected       = false;
    double confidence     = 0.0;
    double trigger_price  = 0.0;
    double suggested_stop = 0.0;    // за high свечи + буфер
};
// -----------------------------------------------------------------------------
// Конфиг детектора.
// -----------------------------------------------------------------------------
struct PinbarSellConfig {
    double min_upper_ratio = 0.60;   // верхняя тень >= 60% range
    double max_body_ratio  = 0.30;   // тело <= 30% range
    double max_lower_ratio = 0.20;   // нижняя тень <= 20% range
    double max_close_pos   = 0.50;   // close в нижней половине
    int    stop_buffer_points = 10;
    double point             = 0.00001;
};
// -----------------------------------------------------------------------------
// PinbarSellDetector — stateless.
// -----------------------------------------------------------------------------
class PinbarSellDetector {
public:
    explicit PinbarSellDetector(PinbarSellConfig cfg = {},
                                CandleGeometryConfig geom_cfg = {});
    [[nodiscard]] PinbarSellSignal detect(const core::Bar& bar) const noexcept;
    const PinbarSellConfig&         config()   const noexcept { return cfg_; }
    const CandleGeometryCalculator& geometry() const noexcept { return geom_; }
private:
    PinbarSellConfig         cfg_;
    CandleGeometryCalculator geom_;
};
} // namespace spartak::patterns