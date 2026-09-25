#include "patterns/PinbarSellDetector.h"
#include <algorithm>
#include <stdexcept>
namespace spartak::patterns {
PinbarSellDetector::PinbarSellDetector(PinbarSellConfig cfg,
                                       CandleGeometryConfig geom_cfg)
    : cfg_(cfg), geom_(geom_cfg) {
    if (cfg_.min_upper_ratio < 0.0 || cfg_.min_upper_ratio > 1.0)
        throw std::invalid_argument("PinbarSellConfig::min_upper_ratio must be in [0,1]");
    if (cfg_.max_body_ratio < 0.0 || cfg_.max_body_ratio > 1.0)
        throw std::invalid_argument("PinbarSellConfig::max_body_ratio must be in [0,1]");
    if (cfg_.max_lower_ratio < 0.0 || cfg_.max_lower_ratio > 1.0)
        throw std::invalid_argument("PinbarSellConfig::max_lower_ratio must be in [0,1]");
    if (cfg_.max_close_pos < 0.0 || cfg_.max_close_pos > 1.0)
        throw std::invalid_argument("PinbarSellConfig::max_close_pos must be in [0,1]");
    if (cfg_.stop_buffer_points < 0)
        throw std::invalid_argument("PinbarSellConfig::stop_buffer_points must be >= 0");
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("PinbarSellConfig::point must be > 0");
}
PinbarSellSignal PinbarSellDetector::detect(const core::Bar& bar) const noexcept {
    PinbarSellSignal sig;
    const auto g = geom_.compute(bar);
    if (g.range <= 0.0) return sig;
    const bool upper_ok = (g.upper_ratio  >= cfg_.min_upper_ratio);
    const bool body_ok  = (g.body_ratio   <= cfg_.max_body_ratio);
    const bool lower_ok = (g.lower_ratio  <= cfg_.max_lower_ratio);
    const bool close_ok = (g.close_position <= cfg_.max_close_pos);
    if (!(upper_ok && body_ok && lower_ok && close_ok)) {
        return sig;
    }
    // confidence — средневзвешенная «уверенность» по 3 метрикам:
    //   - насколько верхняя тень выше порога
    //   - насколько тело меньше порога
    //   - насколько close_position ниже порога
    const double margin_upper =
        (cfg_.min_upper_ratio < 1.0)
            ? (g.upper_ratio - cfg_.min_upper_ratio) / (1.0 - cfg_.min_upper_ratio)
            : 0.0;
    const double margin_body =
        (cfg_.max_body_ratio > 0.0)
            ? (cfg_.max_body_ratio - g.body_ratio) / cfg_.max_body_ratio
            : 0.0;
    const double margin_close =
        (cfg_.max_close_pos > 0.0)
            ? (cfg_.max_close_pos - g.close_position) / cfg_.max_close_pos
            : 0.0;
    auto clamp01 = [](double v) { return std::max(0.0, std::min(1.0, v)); };
    const double c1 = clamp01(margin_upper);
    const double c2 = clamp01(margin_body);
    const double c3 = clamp01(margin_close);
    sig.detected   = true;
    sig.confidence = (c1 + c2 + c3) / 3.0;
    sig.trigger_price  = bar.close;
    sig.suggested_stop = bar.high
                       + static_cast<double>(cfg_.stop_buffer_points) * cfg_.point;
    return sig;
}
} // namespace spartak::patterns