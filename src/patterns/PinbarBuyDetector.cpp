#include "patterns/PinbarBuyDetector.h"
#include <algorithm>
#include <stdexcept>
namespace spartak::patterns {
// -----------------------------------------------------------------------------
// Конструктор с валидацией.
// -----------------------------------------------------------------------------
PinbarBuyDetector::PinbarBuyDetector(PinbarConfig cfg, CandleGeometryConfig geom_cfg)
    : cfg_(cfg), geom_(geom_cfg) {
    if (cfg_.min_lower_ratio < 0.0 || cfg_.min_lower_ratio > 1.0)
        throw std::invalid_argument("PinbarConfig::min_lower_ratio must be in [0,1]");
    if (cfg_.max_body_ratio < 0.0 || cfg_.max_body_ratio > 1.0)
        throw std::invalid_argument("PinbarConfig::max_body_ratio must be in [0,1]");
    if (cfg_.max_upper_ratio < 0.0 || cfg_.max_upper_ratio > 1.0)
        throw std::invalid_argument("PinbarConfig::max_upper_ratio must be in [0,1]");
    if (cfg_.min_close_pos < 0.0 || cfg_.min_close_pos > 1.0)
        throw std::invalid_argument("PinbarConfig::min_close_pos must be in [0,1]");
    if (cfg_.stop_buffer_points < 0)
        throw std::invalid_argument("PinbarConfig::stop_buffer_points must be >= 0");
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("PinbarConfig::point must be > 0");
}
// -----------------------------------------------------------------------------
// detect — проверяет одну свечу.
//
// Алгоритм:
//   1. Считаем геометрию свечи.
//   2. Проверяем 4 условия (lower_ratio, body_ratio, upper_ratio, close_pos).
//   3. Если все ок — заполняем PinbarSignal.
//
// confidence — насколько «идеальным» получился пинбар.
// Берём минимальный «запас» от каждого порога и нормализуем к [0,1].
//
// suggested_stop = bar.low - stop_buffer_points * point
// trigger_price  = bar.close
// -----------------------------------------------------------------------------
PinbarSignal PinbarBuyDetector::detect(const core::Bar& bar) const noexcept {
    PinbarSignal sig;
    const auto g = geom_.compute(bar);
    // Защита от нулевого диапазона
    if (g.range <= 0.0) return sig;
    // 4 условия
    const bool lower_ok = (g.lower_ratio  >= cfg_.min_lower_ratio);
    const bool body_ok  = (g.body_ratio   <= cfg_.max_body_ratio);
    const bool upper_ok = (g.upper_ratio  <= cfg_.max_upper_ratio);
    const bool close_ok = (g.close_position >= cfg_.min_close_pos);
    if (!(lower_ok && body_ok && upper_ok && close_ok)) {
        return sig;   // detected остаётся false
    }
    // confidence: усреднённая «уверенность» по 3 ключевым метрикам:
    //   - насколько нижняя тень длиннее порога
    //   - насколько тело меньше порога
    //   - насколько close_position выше порога
    const double margin_lower =
        (cfg_.min_lower_ratio < 1.0)
            ? (g.lower_ratio - cfg_.min_lower_ratio) / (1.0 - cfg_.min_lower_ratio)
            : 0.0;
    const double margin_body =
        (cfg_.max_body_ratio > 0.0)
            ? (cfg_.max_body_ratio - g.body_ratio) / cfg_.max_body_ratio
            : 0.0;
    const double margin_close =
        (cfg_.min_close_pos < 1.0)
            ? (g.close_position - cfg_.min_close_pos) / (1.0 - cfg_.min_close_pos)
            : 0.0;
    // Ограничиваем маржи в [0,1]
    auto clamp01 = [](double v) {
        return std::max(0.0, std::min(1.0, v));
    };
    const double c1 = clamp01(margin_lower);
    const double c2 = clamp01(margin_body);
    const double c3 = clamp01(margin_close);
    sig.detected   = true;
    sig.confidence = (c1 + c2 + c3) / 3.0;
    sig.trigger_price  = bar.close;
    sig.suggested_stop = bar.low
                       - static_cast<double>(cfg_.stop_buffer_points) * cfg_.point;
    return sig;
}
} // namespace spartak::patterns