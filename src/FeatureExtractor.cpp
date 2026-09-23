#include "../include/FeatureExtractor.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace AHexa {

std::vector<std::vector<double>> FeatureExtractor::extract(const std::vector<Bar>& bars,
                                                            const std::vector<Level>& levels,
                                                            Trend trend,
                                                            const StrategyParams& params) {
    std::vector<std::vector<double>> features;
    if (bars.empty()) return features;

    // Предварительный расчёт EMA (период из params)
    int emaPeriod = params.trendEmaPeriod;
    double k = 2.0 / (emaPeriod + 1);
    std::vector<double> ema(bars.size());
    ema[0] = bars[0].close;
    for (size_t i = 1; i < bars.size(); ++i) {
        ema[i] = bars[i].close * k + ema[i-1] * (1 - k);
    }

    // Для RSI (14 периодов)
    const int rsiPeriod = 14;
    std::vector<double> rsi(bars.size(), 50.0);
    if (bars.size() > rsiPeriod) {
        double gain = 0.0, loss = 0.0;
        for (int i = 1; i <= rsiPeriod; ++i) {
            double diff = bars[i].close - bars[i-1].close;
            if (diff >= 0) gain += diff; else loss -= diff;
        }
        double avgGain = gain / rsiPeriod;
        double avgLoss = loss / rsiPeriod;
        rsi[rsiPeriod] = (avgLoss == 0) ? 100.0 : 100.0 - (100.0 / (1.0 + avgGain / avgLoss));
        for (size_t i = rsiPeriod + 1; i < bars.size(); ++i) {
            double diff = bars[i].close - bars[i-1].close;
            if (diff >= 0) {
                avgGain = (avgGain * (rsiPeriod - 1) + diff) / rsiPeriod;
                avgLoss = (avgLoss * (rsiPeriod - 1)) / rsiPeriod;
            } else {
                avgGain = (avgGain * (rsiPeriod - 1)) / rsiPeriod;
                avgLoss = (avgLoss * (rsiPeriod - 1) - diff) / rsiPeriod;
            }
            rsi[i] = (avgLoss == 0) ? 100.0 : 100.0 - (100.0 / (1.0 + avgGain / avgLoss));
        }
    }

    // Кодировка тренда
    double trendVal = 0.0;
    if (trend == Trend::BULL) trendVal = 1.0;
    else if (trend == Trend::BEAR) trendVal = -1.0;

    // Для каждого бара (начиная с индекса, где есть все данные)
    for (size_t i = emaPeriod; i < bars.size(); ++i) {
        std::vector<double> feat;
        const Bar& b = bars[i];

        // 1. Расстояние до ближайшего уровня (в пунктах) – упрощённо (point = 0.0001)
        double minDist = 1e9;
        for (const auto& lvl : levels) {
            double dist = std::abs(b.close - lvl.price);
            if (dist < minDist) minDist = dist;
        }
        double distPips = minDist / 0.0001; // грубо
        feat.push_back(distPips);

        // 2. RSI
        feat.push_back(rsi[i]);

        // 3. Размер свечи (high-low) в пунктах
        double candleSize = (b.high - b.low) / 0.0001;
        feat.push_back(candleSize);

        // 4. Верхняя тень (high - max(open,close))
        double upperShadow = (b.high - std::max(b.open, b.close)) / 0.0001;
        feat.push_back(upperShadow);

        // 5. Нижняя тень (min(open,close) - low)
        double lowerShadow = (std::min(b.open, b.close) - b.low) / 0.0001;
        feat.push_back(lowerShadow);

        // 6. Положение относительно EMA (close - ema) / point
        double emaDiff = (b.close - ema[i]) / 0.0001;
        feat.push_back(emaDiff);

        // 7. Скорость изменения цены за 5 баров (в пунктах)
        if (i >= 5) {
            double speed = (b.close - bars[i-5].close) / 0.0001;
            feat.push_back(speed);
        } else {
            feat.push_back(0.0);
        }

        // 8. Кодировка тренда
        feat.push_back(trendVal);

        features.push_back(feat);
    }

    return features;
}

void FeatureExtractor::normalize(std::vector<std::vector<double>>& features) {
    if (features.empty() || features[0].empty()) return;
    size_t numFeat = features[0].size();

    for (size_t f = 0; f < numFeat; ++f) {
        double mean = 0.0, stddev = 0.0;
        for (const auto& row : features) mean += row[f];
        mean /= features.size();
        for (const auto& row : features) stddev += (row[f] - mean) * (row[f] - mean);
        stddev = std::sqrt(stddev / features.size());
        if (stddev < 1e-9) continue;
        for (auto& row : features) {
            row[f] = (row[f] - mean) / stddev;
        }
    }
}

} // namespace AHexa