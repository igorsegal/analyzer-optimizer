// =============================================================================
//  SPARTAK :: validation/SlippageToleranceChecker.h
//  Фильтр проскальзывания.
//
//  При открытии ордера цена может отличаться от ожидаемой (slippage).
//  Проверяем: |actual - expected| <= max_points * point.
//
//  Использование:
//    if (!slippage.pass(expected_price, bar_close)) reject(SlippageTooHigh);
// =============================================================================
#pragma once
#include <cstdint>
#include <cmath>
namespace spartak::validation {
// -----------------------------------------------------------------------------
// Конфиг.
// -----------------------------------------------------------------------------
struct SlippageConfig {
    int32_t max_points = 5;
    double  point      = 0.00001;
};
// -----------------------------------------------------------------------------
// SlippageToleranceChecker — stateless.
// -----------------------------------------------------------------------------
class SlippageToleranceChecker {
public:
    explicit SlippageToleranceChecker(SlippageConfig cfg = {});
    // Укладывается ли реальная цена в допуск от ожидаемой.
    [[nodiscard]] bool pass(double expected_price,
                            double actual_price) const noexcept;
    // Проскальзывание в пунктах (абсолютная величина).
    [[nodiscard]] int slippagePoints(double expected_price,
                                     double actual_price) const noexcept;
    const SlippageConfig& config() const noexcept { return cfg_; }
private:
    SlippageConfig cfg_;
};
} // namespace spartak::validation