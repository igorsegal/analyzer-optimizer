// =============================================================================
//  SPARTAK :: patterns/CandleGeometryCalculator.h
//  Базовые метрики одной свечи + статистика по серии.
//
//  Все детекторы паттернов используют эту структуру вместо того,
//  чтобы каждый раз считать body/range/wicks вручную.
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstddef>
#include <vector>
namespace spartak::patterns {
// -----------------------------------------------------------------------------
// Геометрия одной свечи.
// -----------------------------------------------------------------------------
struct CandleGeometry {
    double body           = 0.0;
    double range          = 0.0;
    double upper_wick     = 0.0;
    double lower_wick     = 0.0;
    double body_ratio     = 0.0;
    double upper_ratio    = 0.0;
    double lower_ratio    = 0.0;
    double close_position = 0.0;
    bool   is_bull        = false;
    bool   is_bear        = false;
    bool   is_doji        = false;
};
// -----------------------------------------------------------------------------
// Конфиг: порог doji и период для ATR / среднего range.
// -----------------------------------------------------------------------------
struct CandleGeometryConfig {
    double doji_body_ratio = 0.10;
    int    atr_period      = 14;
};
// -----------------------------------------------------------------------------
// CandleGeometryCalculator — stateless.
// -----------------------------------------------------------------------------
class CandleGeometryCalculator {
public:
    explicit CandleGeometryCalculator(CandleGeometryConfig cfg = {});
    [[nodiscard]] CandleGeometry compute(const core::Bar& bar) const noexcept;
    [[nodiscard]] double atr(const std::vector<core::Bar>& bars,
                             int period = -1) const noexcept;
    [[nodiscard]] double average_range(const std::vector<core::Bar>& bars,
                                       int period = -1) const noexcept;
    const CandleGeometryConfig& config() const noexcept { return cfg_; }
private:
    CandleGeometryConfig cfg_;
};
} // namespace spartak::patterns