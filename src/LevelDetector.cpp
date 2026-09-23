#include "../include/LevelDetector.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace AHexa {

std::vector<Level> LevelDetector::detect(const std::vector<Bar>& bars, int minTouches, double clusterPips) {
    std::vector<Level> levels;
    if (bars.size() < 3) return levels;

    // 1. Находим локальные экстремумы
    std::vector<double> extremumPrices;
    std::vector<Level::Type> extremumTypes;

    for (size_t i = 1; i < bars.size() - 1; ++i) {
        // Локальный максимум
        if (bars[i].high > bars[i-1].high && bars[i].high > bars[i+1].high) {
            extremumPrices.push_back(bars[i].high);
            extremumTypes.push_back(Level::Type::RESISTANCE);
        }
        // Локальный минимум
        if (bars[i].low < bars[i-1].low && bars[i].low < bars[i+1].low) {
            extremumPrices.push_back(bars[i].low);
            extremumTypes.push_back(Level::Type::SUPPORT);
        }
    }

    if (extremumPrices.empty()) return levels;

    // 2. Кластеризация близких цен (порог в пунктах)
    // Предполагаем, что point = 0.0001 для большинства (но можно передавать)
    double tolerance = clusterPips * 0.0001; // упрощённо
    std::vector<bool> used(extremumPrices.size(), false);

    for (size_t i = 0; i < extremumPrices.size(); ++i) {
        if (used[i]) continue;
        Level lvl;
        lvl.price = extremumPrices[i];
        lvl.type = extremumTypes[i];
        lvl.touches = 1;
        lvl.lastTouchTime = 0;
        used[i] = true;

        for (size_t j = i+1; j < extremumPrices.size(); ++j) {
            if (used[j]) continue;
            if (std::abs(extremumPrices[j] - lvl.price) <= tolerance) {
                lvl.touches++;
                used[j] = true;
                // Обновляем цену как среднее
                lvl.price = (lvl.price * (lvl.touches-1) + extremumPrices[j]) / lvl.touches;
            }
        }

        if (lvl.touches >= minTouches) {
            levels.push_back(lvl);
        }
    }

    // Сортируем по силе (количеству касаний) убывание
    std::sort(levels.begin(), levels.end(), [](const Level& a, const Level& b) {
        return a.touches > b.touches;
    });

    return levels;
}

} // namespace AHexa