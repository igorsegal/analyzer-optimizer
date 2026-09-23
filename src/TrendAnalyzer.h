#ifndef TREND_ANALYZER_H
#define TREND_ANALYZER_H

#include "common.h"
#include <vector>

namespace AHexa {

class TrendAnalyzer {
public:
    // Определяет тренд (BULL/BEAR/SIDEWAYS) по бару и периоду EMA
    Trend getTrend(const std::vector<Bar>& bars, int emaPeriod = 50);
};

} // namespace AHexa

#endif