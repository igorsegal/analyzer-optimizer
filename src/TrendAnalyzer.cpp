#include "../include/TrendAnalyzer.h"
#include <vector>

namespace AHexa {

Trend TrendAnalyzer::getTrend(const std::vector<Bar>& bars, int emaPeriod) {
    if (bars.size() < static_cast<size_t>(emaPeriod)) return Trend::SIDEWAYS;

    // Вычисляем EMA
    double k = 2.0 / (emaPeriod + 1);
    double ema = bars[0].close;
    for (size_t i = 1; i < bars.size(); ++i) {
        ema = bars[i].close * k + ema * (1 - k);
    }

    double lastClose = bars.back().close;
    double prevClose = bars[bars.size() - 2].close;

    // Правила:
    // BULL: цена выше EMA и последний максимум выше предыдущего
    if (lastClose > ema && bars.back().high > bars[bars.size() - 2].high) {
        return Trend::BULL;
    }
    // BEAR: цена ниже EMA и последний минимум ниже предыдущего
    if (lastClose < ema && bars.back().low < bars[bars.size() - 2].low) {
        return Trend::BEAR;
    }
    return Trend::SIDEWAYS;
}

} // namespace AHexa