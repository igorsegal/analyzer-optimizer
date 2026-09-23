#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "common.h"
#include <vector>

namespace AHexa {

class WindowManager {
public:
    // Разбивает M5 бары на окна по 4 месяца (3 мес обучение, 1 мес тест)
    std::vector<Window> split(const std::vector<Bar>& bars, int monthsPerWindow = 4);
};

} // namespace AHexa

#endif