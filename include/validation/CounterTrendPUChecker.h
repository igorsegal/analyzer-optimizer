// =============================================================================
//  SPARTAK :: validation/CounterTrendPUChecker.h
//  Проверка, разрешён ли контр-трендовый сигнал.
//
//  По ТЗ «Зри в Корень»:
//    - сигнал ПО тренду            -> всегда разрешён;
//    - сигнал ПРОТИВ тренда        -> разрешён ТОЛЬКО если сигнал сформирован
//                                     на уровне типа IntermediateLevel (ПУ);
//    - тренд не определён           -> разрешён (стратегия сама решает).
// =============================================================================
#pragma once
#include "core/Types.h"
#include <vector>
namespace spartak::validation {
// -----------------------------------------------------------------------------
// Результат проверки.
// -----------------------------------------------------------------------------
struct CounterTrendResult {
    bool allowed    = false;   // итоговое решение
    bool is_counter = false;   // сигнал против тренда?
    bool pu_excuse  = false;   // разрешён через уровень ПУ
};
// -----------------------------------------------------------------------------
// Конфиг.
// -----------------------------------------------------------------------------
struct CounterTrendConfig {
    bool allow_counter_trend_on_pu = true;
    double pu_match_epsilon_points = 2.0;
    double point                   = 0.00001;
};
// -----------------------------------------------------------------------------
// CounterTrendPUChecker — stateless.
// -----------------------------------------------------------------------------
class CounterTrendPUChecker {
public:
    explicit CounterTrendPUChecker(CounterTrendConfig cfg = {});
    [[nodiscard]] CounterTrendResult check(
        core::OrderSide               signal_side,
        core::TrendDirection          trend,
        double                        level,
        const core::MarketContext&    ctx) const noexcept;
    const CounterTrendConfig& config() const noexcept { return cfg_; }
private:
    CounterTrendConfig cfg_;
    bool level_on_pu(double level, const core::MarketContext& ctx) const noexcept;
};
} // namespace spartak::validation