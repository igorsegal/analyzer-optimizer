#include "../include/WindowManager.h"
#include <chrono>
#include <algorithm>

namespace AHexa {

std::vector<Window> WindowManager::split(const std::vector<Bar>& bars, int monthsPerWindow) {
    std::vector<Window> windows;
    if (bars.empty()) return windows;

    // Находим первую и последнюю дату
    int64_t startTime = bars.front().time;
    int64_t endTime = bars.back().time;

    // Конвертируем месяцы в секунды (приближённо: 30.44 дней)
    const int64_t monthSec = 30 * 24 * 3600; // упрощённо 30 дней
    int64_t windowSec = monthsPerWindow * monthSec;
    int64_t trainSec = 3 * monthSec;
    int64_t testSec = 1 * monthSec;

    int64_t currentStart = startTime;
    while (currentStart + windowSec <= endTime) {
        Window win;
        win.startTime = currentStart;
        win.endTime = currentStart + windowSec;

        int64_t trainEnd = currentStart + trainSec;
        int64_t testEnd = currentStart + windowSec;

        // Собираем бары для train
        for (const auto& b : bars) {
            if (b.time >= currentStart && b.time < trainEnd) {
                win.trainBars.push_back(b);
            } else if (b.time >= trainEnd && b.time < testEnd) {
                win.testBars.push_back(b);
            }
        }

        // Проверяем, что есть данные
        if (!win.trainBars.empty() && !win.testBars.empty()) {
            windows.push_back(win);
        }

        currentStart += windowSec;
    }

    return windows;
}

} // namespace AHexa