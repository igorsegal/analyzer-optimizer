#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include "common.h"
#include <vector>

namespace AHexa {

class FeatureExtractor {
public:
    // Извлекает признаки для каждого бара (возвращает матрицу: кол-во баров × кол-во признаков)
    std::vector<std::vector<double>> extract(const std::vector<Bar>& bars,
                                              const std::vector<Level>& levels,
                                              Trend trend,
                                              const StrategyParams& params);
    // Нормализация признаков (Z-score) по обучающей выборке
    void normalize(std::vector<std::vector<double>>& features);
};

} // namespace AHexa

#endif