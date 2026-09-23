#include "block_04_1_consolidation_detector.h"
#include <algorithm>
#include <cmath>
#include <numeric>

// ============================================================================
// КОНСТРУКТОР
// ============================================================================
ConsolidationDetector::ConsolidationDetector(int32_t lookbackBars,
                                             int32_t minSessions,
                                             int32_t barsPerSession,
                                             double rangeRatioThreshold)
    : lookbackBars_(lookbackBars),
      minSessions_(minSessions),
      barsPerSession_(barsPerSession),
      rangeRatioThreshold_(rangeRatioThreshold) {}

// ============================================================================
// ОСНОВНОЙ МЕТОД: Детекция консолидаций (GRAAL)
// ============================================================================
std::vector<Consolidation> ConsolidationDetector::detect(
    const std::vector<XFBAR>& bars,
    const std::optional<std::vector<XFBAR>>& dailyBars) {
    
    std::vector<Consolidation> consolidations;
    
    // GRAAL: Нужен минимальный набор данных для анализа
    if (bars.size() < static_cast<size_t>(lookbackBars_)) {
        return consolidations;
    }
    
    // Окно для анализа (последние lookbackBars)
    auto windowStart = bars.end() - lookbackBars_;
    std::vector<XFBAR> window(windowStart, bars.end());
    
    // 1. Расчёт среднего диапазона бара (для оценки сжатия)
    // GRAAL: Консолидация = сжатый диапазон относительно обычных баров
    double totalRange = 0;
    for (const auto& bar : window) {
        totalRange += (bar.high - bar.low);
    }
    double avgBarRange = totalRange / window.size();
    double maxConsolidationRange = avgBarRange * rangeRatioThreshold_ * 10;
    
    // 2. Минимальная длительность (GRAAL: ?2 основных сессии на H1)
    int32_t minBarsRequired = minSessions_ * barsPerSession_;
    
    // 3. Поиск консолидаций (проторгованных диапазонов)
    // GRAAL: Ищем не экстремумы, а зоны проторговки!
    int32_t i = 0;
    while (i < static_cast<int32_t>(window.size()) - minBarsRequired) {
        int32_t end = findConsolidationEnd(window, i, minBarsRequired, maxConsolidationRange);
        
        if (end > i + minBarsRequired - 1) {
            // Нашли валидную консолидацию
            Consolidation cons;
            cons.start_bar = i;
            cons.end_bar = end;
            cons.duration_bars = end - i + 1;
            
            // Вычисляем диапазон консолидации
            double minLow = window[i].low;
            double maxHigh = window[i].high;
            double totalVolume = 0;
            
            for (int32_t j = i; j <= end; ++j) {
                if (window[j].low < minLow) minLow = window[j].low;
                if (window[j].high > maxHigh) maxHigh = window[j].high;
                totalVolume += window[j].tick_volume;
            }
            
            cons.range_low = minLow;
            cons.range_high = maxHigh;
            cons.avg_volume = totalVolume / cons.duration_bars;
            
            // GRAAL: Daily Context (поддержка в нижней половине дня = сильная)
            cons.daily_context = getDailyContext(
                std::vector<XFBAR>(window.begin() + i, window.begin() + end + 1),
                dailyBars
            );
            
            // GRAAL: Проверка, возвращалась ли цена (сила уровня)
            // Это будет полноценно в Блоке 04.2, но здесь базовая проверка
            cons.is_valid = true;
            
            // GRAAL: Расчёт силы уровня
            cons.strength = calculateStrength(cons, window, dailyBars);
            
            consolidations.push_back(cons);
            
            // Пропускаем эту зону (не ищем вложенные консолидации)
            i = end + 1;
        } else {
            i++;
        }
    }
    
    return consolidations;
}

// ============================================================================
// ПОИСК КОНЦА КОНСОЛИДАЦИИ
// ============================================================================
int32_t ConsolidationDetector::findConsolidationEnd(
    const std::vector<XFBAR>& window,
    int32_t start,
    int32_t minBars,
    double maxRange) {
    
    int32_t end = start + minBars - 1;
    
    while (end < static_cast<int32_t>(window.size())) {
        // Находим диапазон сегмента
        double minLow = window[start].low;
        double maxHigh = window[start].high;
        
        for (int32_t j = start; j <= end; ++j) {
            if (window[j].low < minLow) minLow = window[j].low;
            if (window[j].high > maxHigh) maxHigh = window[j].high;
        }
        
        double segRange = maxHigh - minLow;
        
        // GRAAL: Если диапазон слишком широкий — конец консолидации
        // (цена вышла из проторговки)
        if (segRange > maxRange) {
            break;
        }
        
        end++;
    }
    
    // Проверка минимальной длительности
    if (end - start + 1 >= minBars) {
        return end - 1;
    } else {
        return start;
    }
}

// ============================================================================
// DAILY CONTEXT (GRAAL: Критически важно!)
// ============================================================================
// "Стратегически важные для бычьего тренда поддержки на часовике обычно 
// находятся в нижних частях дневных диапазонов"
// "Чем дальше проторговка на часовике от лоя и ближе к хаю – тем она слабее"
// ============================================================================
std::string ConsolidationDetector::getDailyContext(
    const std::vector<XFBAR>& consolidationWindow,
    const std::optional<std::vector<XFBAR>>& dailyBars) {
    
    // Если нет дневных баров — не можем определить контекст
    if (!dailyBars.has_value() || dailyBars->empty()) {
        return "MIDDLE";
    }
    
    const auto& daily = dailyBars->back();
    double dailyRange = daily.high - daily.low;
    
    if (dailyRange == 0) {
        return "MIDDLE";
    }
    
    double dailyMid = daily.low + dailyRange / 2;
    
    // Средняя цена консолидации
    double consMidSum = 0;
    for (const auto& bar : consolidationWindow) {
        consMidSum += (bar.high + bar.low) / 2;
    }
    double consMidAvg = consMidSum / consolidationWindow.size();
    
    // GRAAL: 
    // - Поддержка в нижней половине дня = сильная (LOWER_HALF)
    // - Сопротивление в верхней половине дня = сильная (UPPER_HALF)
    if (consMidAvg < dailyMid) {
        return "LOWER_HALF";   // Сильно для Поддержки
    } else if (consMidAvg > dailyMid) {
        return "UPPER_HALF";   // Сильно для Сопротивления
    } else {
        return "MIDDLE";
    }
}

// ============================================================================
// ПОДСЧЁТ ЛОЖНЫХ ПРОБОЕВ (GRAAL: Усиливает силу уровня)
// ============================================================================
// "Подд/сопр-я с ярковыраженными завершенными ложными пробитиями 
// имеют больший приоритет"
// ============================================================================
int32_t ConsolidationDetector::countFalseBreakouts(
    const std::vector<XFBAR>& consolidationWindow,
    double rangeLow,
    double rangeHigh) {
    
    int32_t falseBreakouts = 0;
    double threshold = (rangeHigh - rangeLow) * 0.001; // 0.1% за пределы диапазона
    
    for (size_t i = 1; i < consolidationWindow.size() - 1; ++i) {
        const auto& bar = consolidationWindow[i];
        const auto& prevBar = consolidationWindow[i - 1];
        const auto& nextBar = consolidationWindow[i + 1];
        
        // Ложный пробой вверх (вынос за rangeHigh с возвратом)
        if (bar.high > rangeHigh + threshold &&
            prevBar.high <= rangeHigh &&
            nextBar.close < rangeHigh) {
            falseBreakouts++;
        }
        
        // Ложный пробой вниз (вынос за rangeLow с возвратом)
        if (bar.low < rangeLow - threshold &&
            prevBar.low >= rangeLow &&
            nextBar.close > rangeLow) {
            falseBreakouts++;
        }
    }
    
    return falseBreakouts;
}

// ============================================================================
// ПРОВЕРКА: ВОЗВРАЩАЛАСЬ ЛИ ЦЕНА (GRAAL: Критично для силы!)
// ============================================================================
// "Если цена возвращается в зону подд/сопр - она уже не имеет силы"
// "При сильном тренде цена не должна возвращаться в диапазоны, из которых вышла"
// ============================================================================
bool ConsolidationDetector::didPriceReturn(
    const std::vector<XFBAR>& bars,
    int32_t consolidationEnd,
    double rangeLow,
    double rangeHigh) {
    
    // Проверяем бары после конца консолидации
    for (size_t i = consolidationEnd + 1; i < bars.size(); ++i) {
        const auto& bar = bars[i];
        
        // Если цена вернулась в диапазон
        if (bar.low <= rangeHigh && bar.high >= rangeLow) {
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// РАСЧЁТ СИЛЫ УРОВНЯ (GRAAL: Комплексная оценка)
// ============================================================================
// Факторы силы:
// 1. Длительность проторговки (чем дольше > тем сильнее) - макс 0.3
// 2. Ложные пробои внутри (ярковыраженные = больший приоритет) - макс 0.3
// 3. Daily Context (поддержка в нижней половине = +сила) - макс 0.2
// 4. Объём внутри консолидации - макс 0.2
// ============================================================================
double ConsolidationDetector::calculateStrength(
    const Consolidation& consolidation,
    const std::vector<XFBAR>& bars,
    const std::optional<std::vector<XFBAR>>& dailyBars) {
    
    double strength = 0.0;
    
    // 1. Длительность проторговки (GRAAL: "тем сильнее, чем дольше проторговалась цена")
    // 2 сессии = 0.15, 4+ сессии = 0.3
    double durationFactor = static_cast<double>(consolidation.duration_bars) / 
                            (minSessions_ * barsPerSession_ * 2);
    strength += std::min(durationFactor, 0.3);
    
    // 2. Daily Context (GRAAL: критически важно для позиционных сделок)
    // Поддержка в LOWER_HALF или Сопротивление в UPPER_HALF = +0.2
    if (consolidation.daily_context == "LOWER_HALF" || 
        consolidation.daily_context == "UPPER_HALF") {
        strength += 0.2;
    }
    
    // 3. Объём внутри консолидации (GRAAL: "иметь в себе хороший проторгованный объем")
    if (!bars.empty()) {
        double avgVolumeTotal = 0;
        for (const auto& bar : bars) {
            avgVolumeTotal += bar.tick_volume;
        }
        avgVolumeTotal /= bars.size();
        
        if (consolidation.avg_volume >= avgVolumeTotal * 1.5) {
            strength += 0.2;  // Высокий объём = сильная консолидация
        } else if (consolidation.avg_volume >= avgVolumeTotal * 0.5) {
            strength += 0.1;  // Средний объём
        }
    }
    
    // 4. Ложные пробои (GRAAL: "с ярковыраженными ложными пробитиями = больший приоритет")
    // Будет полноценно считаться в Блоке 04.2, здесь базовая оценка
    
    // Ограничиваем силу максимум 1.0
    return std::min(strength, 1.0);
}

// ============================================================================
// ФИЛЬТР ПО ОБЪЁМУ (GRAAL: "Дождаться консолидации с объемом")
// ============================================================================
std::vector<Consolidation> ConsolidationDetector::filterByVolume(
    const std::vector<Consolidation>& consolidations,
    const std::vector<XFBAR>& bars,
    double minVolumeRatio) {
    
    std::vector<Consolidation> filtered;
    
    if (bars.empty() || consolidations.empty()) {
        return consolidations;
    }
    
    // Средний объём по всем барам
    double totalVolume = 0;
    for (const auto& bar : bars) {
        totalVolume += bar.tick_volume;
    }
    double avgVolumeTotal = totalVolume / bars.size();
    
    // GRAAL: Фильтр - объём внутри консолидации должен быть ? X% от среднего
    for (const auto& cons : consolidations) {
        if (cons.avg_volume >= avgVolumeTotal * minVolumeRatio) {
            filtered.push_back(cons);
        }
    }
    
    return filtered;
}