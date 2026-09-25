// =============================================================================
//  SPARTAK :: context/ContextAggregator.h
//  Финальный оркестратор слоя context/.
//
//  Прогоняет всю цепочку:
//    Bar[] -> FractalPointDetector
//          -> ShadowNoiseFilter
//          -> HighExtractor / LowExtractor
//          -> StructureValidatorHHHL
//          -> TrendBiasEvaluator
//          -> PUZoneCalculator / LUZoneCalculator
//          -> OldLevelCleaner
//          -> MarketContext
//
//  Один вызов analyze() — готовый MarketContext для стратегии.
// =============================================================================
#pragma once
#include "core/Types.h"
#include "context/FractalPointDetector.h"
#include "context/ShadowNoiseFilter.h"
#include "context/PUZoneCalculator.h"
#include "context/LUZoneCalculator.h"
#include "context/OldLevelCleaner.h"
#include "context/TrendBiasEvaluator.h"
#include <cstdint>
#include <vector>
namespace spartak::context {
// -----------------------------------------------------------------------------
// Конфиг агрегатора — все параметры слоя context в одном месте.
// -----------------------------------------------------------------------------
struct ContextAggregatorConfig {
    // Fractal
    int  fractal_radius          = 2;
    // Shadow noise
    int  shadow_confirm_window   = 5;
    int  shadow_tolerance_points = 2;
    // Zones
    double point                 = 0.00001;
    int    pu_offset_points      = 15;    // ПУ (Daily)
    int    lu_offset_points      = 10;    // ЛУ (H1)
    std::size_t max_zones_per_tf = 5;
    // Structure / trend
    std::size_t trend_lookback   = 3;
    // Level cleanup
    int64_t max_level_age_ms     = 7LL * 86'400'000LL;
    int     level_break_buffer   = 30;
    // Верхний ТФ используется как «Daily» в тестах (в реальности — отдельный feed)
    bool    use_same_tf_for_both = true;
};
// -----------------------------------------------------------------------------
// ContextAggregator — stateful (держит подсобные объекты).
// -----------------------------------------------------------------------------
class ContextAggregator {
public:
    explicit ContextAggregator(ContextAggregatorConfig cfg = {});
    // Главный вход.
    // older_tf — бары старшего ТФ (Daily)
    // younger_tf — бары младшего ТФ (H1)
    // Если older_tf пустой, используется younger_tf как источник обоих уровней.
    [[nodiscard]] core::MarketContext
    analyze(const std::vector<core::Bar>& older_tf,
            const std::vector<core::Bar>& younger_tf) const;
    const ContextAggregatorConfig& config() const noexcept { return cfg_; }
private:
    ContextAggregatorConfig cfg_;
    FractalPointDetector    fractal_;
    ShadowNoiseFilter       shadow_;
    PUZoneCalculator        pu_calc_;
    LUZoneCalculator        lu_calc_;
    OldLevelCleaner         cleaner_;
    TrendBiasEvaluator      bias_;
};
} // namespace spartak::context