// =============================================================================
//  SPARTAK :: patterns/PatternAggregator.h
//  Финальный оркестратор слоя patterns/.
//
//  Прогоняет все детекторы на последнем баре истории:
//    - TickVolumeFilter   (гейт по объёму)
//    - PinbarBuy/Sell     -> FalseBreakout
//    - EngulfingDetector  -> FalseBreakout
//    - InsideBarDetector  -> Consolidation
//    - ATRImpulseBreakout -> ImpulseBreakout (для каждой активной зоны)
//
//  Возвращает лучший сигнал по приоритету:
//    FalseBreakout > ImpulseBreakout > Consolidation
//
//  Если ничего не сработало — пустой PatternSignal (detected=false).
// =============================================================================
#pragma once
#include "core/Types.h"
#include "context/FractalPointDetector.h"
#include "patterns/CandleGeometryCalculator.h"
#include "patterns/TickVolumeFilter.h"
#include "patterns/PinbarBuyDetector.h"
#include "patterns/PinbarSellDetector.h"
#include "patterns/InsideBarDetector.h"
#include "patterns/EngulfingDetector.h"
#include "patterns/ATRImpulseBreakoutDetector.h"
#include <vector>
namespace spartak::patterns {
// -----------------------------------------------------------------------------
// Конфиг агрегатора — собирает параметры всех детекторов.
// -----------------------------------------------------------------------------
struct PatternAggregatorConfig {
    double point = 0.00001;
    TickVolumeConfig           volume;
    PinbarConfig               pinbar_buy;
    PinbarSellConfig           pinbar_sell;
    InsideBarConfig            inside;
    EngulfingConfig            engulfing;
    ImpulseConfig              impulse;
    CandleGeometryConfig       geometry;
    // Требовать подтверждения структуры тренда для Consolidation:
    // (Inside Bar превращается в сигнал только если тренд определён).
    bool require_trend_for_consolidation = true;
};
// -----------------------------------------------------------------------------
// PatternAggregator — stateful (держит объекты детекторов).
// -----------------------------------------------------------------------------
class PatternAggregator {
public:
    explicit PatternAggregator(PatternAggregatorConfig cfg = {});
    // bars — история с текущим баром последним
    // ctx  — MarketContext из слоя context/
    [[nodiscard]] core::PatternSignal analyze(
        const std::vector<core::Bar>& bars,
        const core::MarketContext&    ctx) const;
    const PatternAggregatorConfig& config() const noexcept { return cfg_; }
private:
    PatternAggregatorConfig           cfg_;
    CandleGeometryCalculator          geom_;
    TickVolumeFilter                  volume_;
    PinbarBuyDetector                 pinbar_buy_;
    PinbarSellDetector                pinbar_sell_;
    InsideBarDetector                 inside_;
    EngulfingDetector                 engulfing_;
    ATRImpulseBreakoutDetector        impulse_;
};
} // namespace spartak::patterns