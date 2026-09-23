#include "../include/MLPredictor.h"
#include <cmath>
#include <random>
#include <algorithm>

namespace AHexa {

MLPredictor::MLPredictor(int numFeatures, double learningRate, int epochs)
    : lr(learningRate), epochs(epochs) {
    // Инициализация весов случайными малыми значениями
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> dist(0.0, 0.01);
    weights.resize(numFeatures);
    for (auto& w : weights) w = dist(gen);
    bias = 0.0;
}

double MLPredictor::sigmoid(double z) const {
    return 1.0 / (1.0 + std::exp(-z));
}

void MLPredictor::train(const std::vector<std::vector<double>>& features,
                        const std::vector<int>& targets) {
    if (features.empty() || targets.empty()) return;
    size_t n = features.size();
    size_t m = features[0].size();
    if (weights.size() != m) weights.resize(m, 0.0);

    // Градиентный спуск (SGD)
    for (int epoch = 0; epoch < epochs; ++epoch) {
        double totalLoss = 0.0;
        std::vector<double> gradW(m, 0.0);
        double gradB = 0.0;

        for (size_t i = 0; i < n; ++i) {
            double z = bias;
            for (size_t j = 0; j < m; ++j) z += weights[j] * features[i][j];
            double pred = sigmoid(z);
            double error = pred - targets[i];
            totalLoss += -targets[i] * std::log(pred + 1e-9) - (1 - targets[i]) * std::log(1 - pred + 1e-9);
            for (size_t j = 0; j < m; ++j) gradW[j] += error * features[i][j];
            gradB += error;
        }

        // Обновление весов
        for (size_t j = 0; j < m; ++j) weights[j] -= lr * gradW[j] / n;
        bias -= lr * gradB / n;

        // Ранняя остановка (если ошибка мала)
        if (totalLoss / n < 0.01) break;
    }
}

double MLPredictor::predict(const std::vector<double>& feature) const {
    double z = bias;
    for (size_t j = 0; j < feature.size(); ++j) {
        if (j < weights.size()) z += weights[j] * feature[j];
    }
    return sigmoid(z);
}

} // namespace AHexa