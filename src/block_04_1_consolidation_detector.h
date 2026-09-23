#pragma once

#include "structures.h"
#include <vector>
#include <optional>

// ============================================================================
// БЛОК 04.1: ДЕТЕКТОР КОНСОЛИДАЦИЙ (GRAAL)
// ============================================================================
// Логика по документам GRAAL:
// 1. Консолидация = проторгованный диапазон (НЕ экстремумы!)
// 2. Минимум 2 основные сессии (H1) или 1 неделя (D1)
// 3. Внутри должен быть накопленный объём
// 4. Daily Context: поддержка в нижней половине дня = сильная
// 5. Если цена вернулась в диапазон > сила утрачена (Блок 04.2)
// ============================================================================

class ConsolidationDetector {
public:
    // Конструктор с параметрами по GRAAL
    ConsolidationDetector(int32_t lookbackBars = 100,
                          int32_t minSessions = 2,
                          int32_t barsPerSession = 12,
                          double rangeRatioThreshold = 0.5);
    
    // ========================================================================
    // ОСНОВНОЙ МЕТОД: Детекция консолидаций
    // ========================================================================
    // bars: массив баров текущего ТФ (например, H1 или M5)
    // dailyBars: дневные бары для контекста (опционально, но рекомендуется)
    // Возвращает: вектор найденных консолидаций
    // ========================================================================
    std::vector<Consolidation> detect(
        const std::vector<XFBAR>& bars,
        const std::optional<std::vector<XFBAR>>& dailyBars = std::nullopt
    );
    
    // ========================================================================
    // ФИЛЬТР ПО ОБЪЁМУ (GRAAL: "Дождаться консолидации с объемом")
    // ========================================================================
    // consolidations: найденные консолидации
    // bars: исходные бары для расчёта среднего объёма
    // minVolumeRatio: мин. отношение к среднему объёму (по умолчанию 0.3 = 30%)
    // Возвращает: отфильтрованный вектор консолидаций
    // ========================================================================
    std::vector<Consolidation> filterByVolume(
        const std::vector<Consolidation>& consolidations,
        const std::vector<XFBAR>& bars,
        double minVolumeRatio = 0.3
    );
    
    // ========================================================================
    // РАСЧЁТ СИЛЫ УРОВНЯ (GRAAL: факторы силы)
    // ========================================================================
    // Факторы:
    // 1. Длительность проторговки (чем дольше > тем сильнее)
    // 2. Ложные пробои внутри (ярковыраженные = больший приоритет)
    // 3. Daily Context (поддержка в нижней половине = +сила)
    // 4. Близость к текущей цене (чем ближе > тем выше приоритет)
    // ========================================================================
    double calculateStrength(
        const Consolidation& consolidation,
        const std::vector<XFBAR>& bars,
        const std::optional<std::vector<XFBAR>>& dailyBars
    );

private:
    int32_t lookbackBars_;
    int32_t minSessions_;
    int32_t barsPerSession_;
    double rangeRatioThreshold_;
    
    // ========================================================================
    // ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
    // ========================================================================
    
    // Поиск конца консолидации (где диапазон становится слишком широким)
    int32_t findConsolidationEnd(
        const std::vector<XFBAR>& window,
        int32_t start,
        int32_t minBars,
        double maxRange
    );
    
    // Daily Context по GRAAL:
    // - Поддержка в нижней половине дня = сильная (LOWER_HALF)
    // - Сопротивление в верхней половине дня = сильная (UPPER_HALF)
    std::string getDailyContext(
        const std::vector<XFBAR>& consolidationWindow,
        const std::optional<std::vector<XFBAR>>& dailyBars
    );
    
    // Подсчёт ложных пробоев внутри консолидации
    // (GRAAL: "с ярковыраженными завершенными ложными пробитиями = больший приоритет")
    int32_t countFalseBreakouts(
        const std::vector<XFBAR>& consolidationWindow,
        double rangeLow,
        double rangeHigh
    );
    
    // Проверка: возвращалась ли цена в диапазон после выхода
    // (GRAAL: "Если цена возвращается в зону подд/сопр - она уже не имеет силы")
    bool didPriceReturn(
        const std::vector<XFBAR>& bars,
        int32_t consolidationEnd,
        double rangeLow,
        double rangeHigh
    );
};