#ifndef LEVEL_DETECTOR_H
#define LEVEL_DETECTOR_H

#include "common.h"
#include <vector>

namespace AHexa {

class LevelDetector {
public:
    // Находит уровни поддержки/сопротивления на баре (Daily или H1)
    std::vector<Level> detect(const std::vector<Bar>& bars, 
                              int minTouches = 3, 
                              double clusterPips = 5.0);
};

} // namespace AHexa

#endif