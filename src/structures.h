#pragma once

#include <cstdint>
#include <string>
#include <vector>

// ============================================================================
// XFBAR ФОРМАТ (60 байт) - точное соответствие .bin файлу
// ============================================================================
#pragma pack(push, 1)
struct XFBAR {
    int64_t datetime;      // 8 байт - Unix timestamp
    double  open;          // 8 байт
    double  high;          // 8 байт
    double  low;           // 8 байт
    double  close;         // 8 байт
    int64_t tick_volume;   // 8 байт
    int32_t spread;        // 4 байт
    int64_t real_volume;   // 8 байт
    // Итого: 60 байт
};
#pragma pack(pop)

// ============================================================================
// КОНСОЛИДАЦИЯ (по GRAAL)
// Проторгованный диапазон с открытыми убыточными позициями
// ============================================================================
struct Consolidation {
    int32_t     start_bar;       // Индекс начала консолидации
    int32_t     end_bar;         // Индекс конца консолидации
    double      range_low;       // Минимум диапазона
    double      range_high;      // Максимум диапазона
    int32_t     duration_bars;   // Длительность в барах
    double      avg_volume;      // Средний объём внутри
    std::string daily_context;   // "LOWER_HALF" / "UPPER_HALF" / "MIDDLE"
    bool        is_valid;        // true = цена не возвращалась (GRAAL: имеет силу)
    double      strength;        // 0.0 - 1.0 (расчётная сила)
    
    Consolidation() 
        : start_bar(0), end_bar(0), range_low(0), range_high(0),
          duration_bars(0), avg_volume(0), daily_context("MIDDLE"),
          is_valid(true), strength(0.5) {}
};

// ============================================================================
// УРОВЕНЬ (Support / Resistance)
// ============================================================================
enum class LevelType {
    SUPPORT,
    RESISTANCE,
    NONE
};

struct Level {
    LevelType       type;              // SUPPORT / RESISTANCE
    double          range_low;         // Нижняя граница
    double          range_high;        // Верхняя граница
    double          strength;          // 0.0 - 1.0
    int32_t         false_breakouts;   // Количество ложных пробоев
    bool            is_active;         // true = цена не возвращалась
    std::string     daily_context;     // "LOWER_HALF" / "UPPER_HALF" / "MIDDLE"
    
    Level() 
        : type(LevelType::NONE), range_low(0), range_high(0),
          strength(0), false_breakouts(0), is_active(true),
          daily_context("MIDDLE") {}
};

// ============================================================================
// БАЛАНС РЫНКА (GRAAL: кто контролирует)
// ============================================================================
enum class BalanceState {
    BULLISH,     // Сильные деньги в лонгах
    BEARISH,     // Сильные деньги в шортах
    NEUTRAL      // Невозможно определить (пила/боковик)
};

struct Balance {
    BalanceState  state;
    double        confidence;    // 0.0 - 1.0
    std::string   reason;
    
    Balance() 
        : state(BalanceState::NEUTRAL), confidence(1.0), reason("No data") {}
};

// ============================================================================
// СИГНАЛ НА ВХОД
// ============================================================================
enum class SignalAction {
    BUY,
    SELL,
    NONE
};

struct Signal {
    SignalAction  direction;
    double        entry;
    double        sl;
    double        tp;
    double        confidence;
    std::string   pattern;       // "TEST_100", "TEST_50", "REVERSAL"
    std::string   reason;
    
    Signal() 
        : direction(SignalAction::NONE), entry(0), sl(0), tp(0),
          confidence(0), pattern(""), reason("") {}
};

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================
inline std::string balanceStateToString(BalanceState state) {
    switch (state) {
        case BalanceState::BULLISH:  return "BULLISH";
        case BalanceState::BEARISH:  return "BEARISH";
        case BalanceState::NEUTRAL:  return "NEUTRAL";
        default:                     return "UNKNOWN";
    }
}

inline std::string levelTypeToString(LevelType type) {
    switch (type) {
        case LevelType::SUPPORT:     return "SUPPORT";
        case LevelType::RESISTANCE:  return "RESISTANCE";
        default:                     return "NONE";
    }
}

inline std::string signalActionToString(SignalAction action) {
    switch (action) {
        case SignalAction::BUY:  return "BUY";
        case SignalAction::SELL: return "SELL";
        default:                 return "NONE";
    }
}