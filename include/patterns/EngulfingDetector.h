// =============================================================================
//  SPARTAK :: patterns/EngulfingDetector.h
//  Детектор Engulfing (поглощение) — Bullish и Bearish.
//
//  Логика:
//    Bullish Engulfing:
//      prev — медвежья (close < open)
//      curr — бычья    (close > open)
//      curr.open  <= prev.close   (открытие ниже закрытия prev)
//      curr.close >= prev.open    (закрытие выше открытия prev)
//      body(curr) >= body(prev) * min_body_ratio_over_prev
//
//    Bearish Engulfing — зеркально:
//      prev — бычья, curr — медвежья
//      curr.open  >= prev.close
//      curr.close <= prev.open
// =============================================================================
#pragma once
#include "core/Types.h"
#include <vector>
namespace spartak::patterns {
// -----------------------------------------------------------------------------
// Результат детекции.
// -----------------------------------------------------------------------------
struct EngulfingSignal {
    bool   detected       = false;
    bool   is_bullish     = false;   // true = Bullish, false = Bearish
    double confidence     = 0.0;     // 0..1
    double trigger_price  = 0.0;     // close текущей свечи
    double suggested_stop = 0.0;     // за экстремум текущей свечи с буфером
};
// -----------------------------------------------------------------------------
// Конфиг детектора.
// -----------------------------------------------------------------------------
struct EngulfingConfig {
    double min_body_ratio_over_prev = 1.0;   // body(curr) / body(prev)
    int    stop_buffer_points       = 10;
    double point                    = 0.00001;
};
// -----------------------------------------------------------------------------
// EngulfingDetector — stateless.
// -----------------------------------------------------------------------------
class EngulfingDetector {
public:
    explicit EngulfingDetector(EngulfingConfig cfg = {});
    // Проверка двух последовательных свечей.
    [[nodiscard]] EngulfingSignal detect(const core::Bar& prev,
                                         const core::Bar& curr) const noexcept;
    // Последняя свеча в истории.
    [[nodiscard]] EngulfingSignal detectLast(const std::vector<core::Bar>& bars) const noexcept;
    const EngulfingConfig& config() const noexcept { return cfg_; }
private:
    EngulfingConfig cfg_;
};
} // namespace spartak::patterns