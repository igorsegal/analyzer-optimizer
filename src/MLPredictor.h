#ifndef ML_PREDICTOR_H
#define ML_PREDICTOR_H

#include <vector>

namespace AHexa {

class MLPredictor {
public:
    MLPredictor(int numFeatures = 8, double learningRate = 0.01, int epochs = 1000);

    void train(const std::vector<std::vector<double>>& features,
               const std::vector<int>& targets);

    double predict(const std::vector<double>& feature) const;

private:
    std::vector<double> weights;
    double bias;
    double lr;
    int epochs;
    double sigmoid(double z) const;
};

} // namespace AHexa

#endif