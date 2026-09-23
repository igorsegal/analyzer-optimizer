#ifndef SIGNAL_GENERATOR_H
#define SIGNAL_GENERATOR_H

#include "common.h"
#include <vector>

namespace AHexa {

class SignalGenerator {
public:
    // Генерирует сигнал (BUY/SELL/NONE) на основе текущего бара, уровней, тренда и ML-вероятности
    Signal generate(const Bar& currentBar,
                    const std::vector<Level>& levels,
                    Trend trend,
                    double mlProb,
                    const StrategyParams& params);
};

} // namespace AHexa

#endif