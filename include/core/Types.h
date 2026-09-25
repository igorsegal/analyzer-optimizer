// =============================================================================
//  SPARTAK :: core/Types.h
//  Базовые типы данных для всего конвейера стратегии «Зри в Корень» (ТАП).
//
//  Файл содержит ТОЛЬКО структуры и enum'ы, без логики.
//  Все модули бэктестера обмениваются через эти типы снизу вверх:
//
//     Bar / Tick  ->  MarketContext  ->  PatternSignal  ->  ValidatedOrderRequest
// =============================================================================
#pragma once
#include <cstdint>
#include <vector>
namespace spartak::core {
// -----------------------------------------------------------------------------
// 1. Рыночные данные
// -----------------------------------------------------------------------------
struct Bar {
    int64_t timestamp   = 0;   // Unix ms (открытие свечи)
    double  open        = 0.0;
    double  high        = 0.0;
    double  low         = 0.0;
    double  close       = 0.0;
    int64_t tick_volume = 0;
    int32_t spread      = 0;   // спред в пунктах
};
struct Tick {
    int64_t timestamp     = 0;
    double  bid           = 0.0;
    double  ask           = 0.0;
    int32_t spread_points = 0;
};
// -----------------------------------------------------------------------------
// 2. Тренд и уровни
// -----------------------------------------------------------------------------
enum class TrendDirection {
    Undefined,
    Bullish,   // БТ
    Bearish    // МТ
};
enum class LevelType {
    LocalLevel,          // ЛУ
    IntermediateLevel    // ПУ
};
struct PriceZone {
    LevelType type           = LevelType::LocalLevel;
    int64_t   formation_time = 0;      // Unix ms — когда уровень сформировался
    double    price_level    = 0.0;
    double    zone_top       = 0.0;
    double    zone_bottom    = 0.0;
    bool      is_active      = true;
};
// -----------------------------------------------------------------------------
// 3. Контекст рынка (выход ContextAggregator)
// -----------------------------------------------------------------------------
struct MarketContext {
    TrendDirection         daily_trend     = TrendDirection::Undefined;
    TrendDirection         hourly_trend    = TrendDirection::Undefined;
    TrendDirection         dominant_trend  = TrendDirection::Undefined;
    bool                   trend_conflict  = false;
    std::vector<PriceZone> active_zones;
    bool                   has_hh_hl_structure = false;
};
// -----------------------------------------------------------------------------
// 4. Направление торговой позиции
// -----------------------------------------------------------------------------
// ВАЖНО: объявлено ДО PatternSignal — последний использует OrderSide.
// -----------------------------------------------------------------------------
enum class OrderSide {
    Buy,
    Sell
};
// -----------------------------------------------------------------------------
// 5. Паттерн (выход PatternAggregator)
// -----------------------------------------------------------------------------
enum class PatternType {
    None,
    FalseBreakout,
    Consolidation,
    ImpulseBreakout
};
struct PatternSignal {
    bool        detected       = false;
    OrderSide   side           = OrderSide::Buy;
    PatternType type           = PatternType::None;
    double      trigger_price  = 0.0;
    double      level          = 0.0;
    double      suggested_stop = 0.0;
    double      confidence     = 0.0;
};
// -----------------------------------------------------------------------------
// 6. Одобренная заявка (выход SignalValidator)
// -----------------------------------------------------------------------------
struct ValidatedOrderRequest {
    bool      is_approved   = false;
    OrderSide side          = OrderSide::Buy;
    double    volume        = 0.0;
    double    entry_price   = 0.0;
    double    stop_loss     = 0.0;
    double    take_profit_1 = 0.0;
    double    take_profit_2 = 0.0;
};
} // namespace spartak::core