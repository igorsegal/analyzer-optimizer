#ifndef AHEXA_COMMON_H
#define AHEXA_COMMON_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace AHexa {

// === Перечисления ===

// Таймфреймы
enum class TimeFrame {
    M5,
    M15,
    H1,
    H4,
    Daily
};

// Направление тренда
enum class Trend {
    BULL,
    BEAR,
    SIDEWAYS
};

// === Структуры данных ===

// Бар (свеча) - соответствует формату XFBAR
struct Bar {
    int64_t  time;          // Unix timestamp (секунды)
    double   open;
    double   high;
    double   low;
    double   close;
    int64_t  tick_volume;
    int32_t  spread;
    int64_t  real_volume;
};

// Уровень поддержки/сопротивления
struct Level {
    enum class Type { SUPPORT, RESISTANCE };
    Type    type;
    double  price;
    int     touches;           // количество касаний
    int64_t lastTouchTime;     // время последнего касания
};

// Торговый сигнал
struct Signal {
    enum class Direction { BUY, SELL, NONE };
    Direction direction;
    double   entryPrice;
    double   stopLoss;
    double   takeProfit;
};

// Информация о одной сделке
struct Trade {
    int64_t openTime;
    int64_t closeTime;
    double  entryPrice;
    double  exitPrice;
    double  profit;          // в валюте депозита
    double  profitPips;
    bool    win;
};

// Статистика за период (окно или весь инструмент)
struct Statistics {
    double totalProfit = 0.0;
    double totalProfitPips = 0.0;
    double maxDrawdown = 0.0;
    double winRate = 0.0;
    double profitFactor = 0.0;
    double sharpeRatio = 0.0;
    int    totalTrades = 0;
    int    winTrades = 0;
    double avgProfit = 0.0;
    double avgLoss = 0.0;
};

// Параметры стратегии (оптимизируемые)
struct StrategyParams {
    int    trendEmaPeriod = 50;       // период EMA для тренда
    double levelTolerancePips = 10.0; // допуск к уровню в пунктах
    int    minTouches = 3;            // минимальное число касаний для уровня
    int    confirmationBars = 1;      // число свечей для подтверждения
    double takeProfitRatio = 2.0;     // отношение TP к SL
    double breakEvenPips = 20.0;      // активация безубытка (в пунктах)
    double mlThreshold = 0.7;         // порог вероятности ML
    int    mlLookaheadBars = 5;       // горизонт для разворота
};

// Окно (обучение + тест)
struct Window {
    std::vector<Bar> trainBars;   // 3 месяца
    std::vector<Bar> testBars;    // 1 месяц
    int64_t startTime;
    int64_t endTime;
};

// Результат оптимизации для одного окна
struct WindowResult {
    StrategyParams params;
    Statistics     trainStats;
    Statistics     testStats;
};

// Информация об инструменте (для сканирования)
struct InstrumentInfo {
    std::string symbol;
    std::string filePath;
    std::string group;          // "Spot", "Crypto", "Metals" (заполняется позже)
};

// Итоговый результат по инструменту
struct InstrumentResult {
    std::string symbol;
    std::string group;
    std::vector<WindowResult> windowResults;
    Statistics avgTrainStats;
    Statistics avgTestStats;
    StrategyParams bestOverallParams;
};

// Выбранный инструмент для итогового отчёта
struct SelectedInstrument {
    std::string symbol;
    std::string group;
    StrategyParams params;
    Statistics stats;
};

// === Глобальные константы ===

const std::string DATA_ROOT = "D:/AHexaTrader/DataFiles/raw/";
const std::string OUTPUT_CSV = "top50_pairs.csv";

// Размер записи баров в XFBAR (фиксирован)
const int RECORD_SIZE = 60;

// Магическое число заголовка
const char MAGIC[8] = "XFBAR001";

} // namespace AHexa

#endif // AHEXA_COMMON_H