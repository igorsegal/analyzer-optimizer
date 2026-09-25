#include "validation/SlippageToleranceChecker.h"
#include <stdexcept>
namespace spartak::validation {
SlippageToleranceChecker::SlippageToleranceChecker(SlippageConfig cfg)
    : cfg_(cfg) {
    if (cfg_.max_points < 0)
        throw std::invalid_argument("SlippageConfig::max_points must be >= 0");
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("SlippageConfig::point must be > 0");
}
// -----------------------------------------------------------------------------
// slippagePoints — абсолютный slippage в пунктах.
// -----------------------------------------------------------------------------
int SlippageToleranceChecker::slippagePoints(double expected_price,
                                             double actual_price) const noexcept {
    const double diff = std::fabs(actual_price - expected_price);
    return static_cast<int>(diff / cfg_.point + 0.5);   // округление до целого
}
// -----------------------------------------------------------------------------
// pass — в пределах допуска?
// -----------------------------------------------------------------------------
bool SlippageToleranceChecker::pass(double expected_price,
                                    double actual_price) const noexcept {
    return slippagePoints(expected_price, actual_price) <= cfg_.max_points;
}
} // namespace spartak::validation