#include "context/ContextAggregator.h"
#include "context/HighExtractor.h"
#include "context/LowExtractor.h"
#include "context/StructureValidatorHHHL.h"
#include <algorithm>
#include <stdexcept>
namespace spartak::context {
// -----------------------------------------------------------------------------
// Конструктор: собираем все подобъекты из единого конфига.
// -----------------------------------------------------------------------------
ContextAggregator::ContextAggregator(ContextAggregatorConfig cfg)
    : cfg_(cfg),
      fractal_(cfg.fractal_radius),
      shadow_([&]{
          ShadowNoiseConfig s;
          s.confirm_window   = cfg.shadow_confirm_window;
          s.tolerance_points = cfg.shadow_tolerance_points;
          s.point            = cfg.point;
          return s;
      }()),
      pu_calc_(cfg.point, cfg.pu_offset_points),
      lu_calc_(cfg.point, cfg.lu_offset_points),
      cleaner_([&]{
          OldLevelConfig o;
          o.max_age_ms          = cfg.max_level_age_ms;
          o.break_buffer_points = cfg.level_break_buffer;
          o.point               = cfg.point;
          return o;
      }()),
      bias_(cfg.trend_lookback)
{
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("ContextAggregatorConfig::point must be > 0");
    if (cfg_.fractal_radius < 1)
        throw std::invalid_argument("ContextAggregatorConfig::fractal_radius must be >= 1");
    if (cfg_.trend_lookback < 2)
        throw std::invalid_argument("ContextAggregatorConfig::trend_lookback must be >= 2");
}
// -----------------------------------------------------------------------------
// analyze — полная цепочка.
// -----------------------------------------------------------------------------
core::MarketContext
ContextAggregator::analyze(const std::vector<core::Bar>& older_tf,
                           const std::vector<core::Bar>& younger_tf) const
{
    core::MarketContext ctx;
    // Определяем, какие бары использовать для Daily и H1.
    // Если older_tf пуст — используем younger_tf как источник обоих.
    const std::vector<core::Bar>& daily_bars =
        (cfg_.use_same_tf_for_both || older_tf.empty()) ? younger_tf : older_tf;
    const std::vector<core::Bar>& hourly_bars = younger_tf;
    // Ничего не считаем, если нет данных
    if (hourly_bars.empty()) return ctx;
    // ---------- 1. Fractals на обоих ТФ ----------
    auto daily_raw  = fractal_.find(daily_bars);
    auto hourly_raw = fractal_.find(hourly_bars);
    // ---------- 2. Shadow noise filter ----------
    auto daily_clean  = shadow_.filter(daily_bars,  daily_raw);
    auto hourly_clean = shadow_.filter(hourly_bars, hourly_raw);
    // ---------- 3. Разделение High / Low ----------
    auto d_highs = HighExtractor::extract(daily_clean);
    auto d_lows  = LowExtractor::extract(daily_clean);
    auto h_highs = HighExtractor::extract(hourly_clean);
    auto h_lows  = LowExtractor::extract(hourly_clean);
    // ---------- 4. Классификация трендов по каждому ТФ ----------
    const auto d_struct = StructureValidatorHHHL::validate(
        d_highs, d_lows, cfg_.trend_lookback);
    const auto h_struct = StructureValidatorHHHL::validate(
        h_highs, h_lows, cfg_.trend_lookback);
    auto struct_to_dir = [](StructureType s) {
        switch (s) {
            case StructureType::Bullish: return core::TrendDirection::Bullish;
            case StructureType::Bearish: return core::TrendDirection::Bearish;
            default:                     return core::TrendDirection::Undefined;
        }
    };
    ctx.daily_trend  = struct_to_dir(d_struct);
    ctx.hourly_trend = struct_to_dir(h_struct);
    // ---------- 5. Доминирующий тренд через TrendBiasEvaluator ----------
    const auto bias = bias_.evaluate(d_highs, d_lows, h_highs, h_lows);
    ctx.dominant_trend  = bias.direction;
    ctx.trend_conflict  = bias.conflict;
    ctx.has_hh_hl_structure =
        (ctx.daily_trend  != core::TrendDirection::Undefined) ||
        (ctx.hourly_trend != core::TrendDirection::Undefined);
    // ---------- 6. Зоны: PU (Daily) + LU (H1) ----------
    auto pu_zones = pu_calc_.build(daily_clean,  cfg_.max_zones_per_tf);
    auto lu_zones = lu_calc_.build(hourly_clean, cfg_.max_zones_per_tf);
    ctx.active_zones.reserve(pu_zones.size() + lu_zones.size());
    ctx.active_zones.insert(ctx.active_zones.end(),
                            pu_zones.begin(), pu_zones.end());
    ctx.active_zones.insert(ctx.active_zones.end(),
                            lu_zones.begin(), lu_zones.end());
    // ---------- 7. Очистка устаревших зон ----------
    const int64_t now_ms      = hourly_bars.back().timestamp;
    const double  last_close  = hourly_bars.back().close;
    cleaner_.cleanInPlace(ctx.active_zones, now_ms, last_close);
    return ctx;
}
} // namespace spartak::context