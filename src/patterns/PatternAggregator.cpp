#include "patterns/PatternAggregator.h"
#include <algorithm>
#include <stdexcept>
namespace spartak::patterns {
// -----------------------------------------------------------------------------
// Конструктор: собираем все детекторы из единого конфига.
// -----------------------------------------------------------------------------
PatternAggregator::PatternAggregator(PatternAggregatorConfig cfg)
    : cfg_(cfg),
      geom_(cfg.geometry),
      volume_(cfg.volume),
      pinbar_buy_(cfg.pinbar_buy, cfg.geometry),
      pinbar_sell_(cfg.pinbar_sell, cfg.geometry),
      inside_(cfg.inside),
      engulfing_(cfg.engulfing),
      impulse_(cfg.impulse, cfg.geometry)
{
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("PatternAggregatorConfig::point must be > 0");
}
// -----------------------------------------------------------------------------
// analyze — полный проход детекторов, выбор лучшего сигнала.
// -----------------------------------------------------------------------------
core::PatternSignal PatternAggregator::analyze(
        const std::vector<core::Bar>& bars,
        const core::MarketContext&    ctx) const
{
    core::PatternSignal best;   // detected=false по умолчанию
    if (bars.empty()) return best;
    const core::Bar& bar = bars.back();
    // --- 0. Гейт по объёму ---
    if (!volume_.passAll(bar, bars)) return best;
    // --- 1. Pinbar BUY/SELL на активных зонах -> FalseBreakout ---
    for (const auto& zone : ctx.active_zones) {
        if (!zone.is_active) continue;
        // Pinbar BUY (отскок от поддержки вверх)
        if (bar.low <= zone.zone_top && bar.high >= zone.zone_bottom) {
            auto pb = pinbar_buy_.detect(bar);
            if (pb.detected) {
                core::PatternSignal s;
                s.detected      = true;
                s.side          = core::OrderSide::Buy;
                s.type          = core::PatternType::FalseBreakout;
                s.trigger_price = pb.trigger_price;
                s.level         = zone.price_level;
                s.suggested_stop = pb.suggested_stop;
                s.confidence    = pb.confidence;
                return s;   // приоритет №1 — возвращаем сразу
            }
            // Pinbar SELL (отскок от сопротивления вниз)
            auto ps = pinbar_sell_.detect(bar);
            if (ps.detected) {
                core::PatternSignal s;
                s.detected      = true;
                s.side          = core::OrderSide::Sell;
                s.type          = core::PatternType::FalseBreakout;
                s.trigger_price = ps.trigger_price;
                s.level         = zone.price_level;
                s.suggested_stop = ps.suggested_stop;
                s.confidence    = ps.confidence;
                return s;
            }
        }
    }
    // --- 2. Engulfing -> FalseBreakout ---
    if (bars.size() >= 2) {
        auto eng = engulfing_.detectLast(bars);
        if (eng.detected) {
            core::PatternSignal s;
            s.detected      = true;
            s.side          = eng.is_bullish ? core::OrderSide::Buy
                                             : core::OrderSide::Sell;
            s.type          = core::PatternType::FalseBreakout;
            s.trigger_price = eng.trigger_price;
            s.level         = eng.trigger_price;   // уровень = точка входа
            s.suggested_stop = eng.suggested_stop;
            s.confidence    = eng.confidence;
            return s;
        }
    }
    // --- 3. ATRImpulse по каждой активной зоне -> ImpulseBreakout ---
    for (const auto& zone : ctx.active_zones) {
        if (!zone.is_active) continue;
        auto imp = impulse_.detect(bars, zone.price_level);
        if (imp.detected) {
            core::PatternSignal s;
            s.detected      = true;
            s.side          = imp.is_bullish ? core::OrderSide::Buy
                                             : core::OrderSide::Sell;
            s.type          = core::PatternType::ImpulseBreakout;
            s.trigger_price = imp.trigger_price;
            s.level         = zone.price_level;
            s.suggested_stop = imp.suggested_stop;
            s.confidence    = imp.confidence;
            // держим лучший из всех зон
            if (!best.detected || s.confidence > best.confidence) {
                best = s;
            }
        }
    }
    if (best.detected) return best;
    // --- 4. InsideBar -> Consolidation ---
    if (bars.size() >= 2) {
        auto ib = inside_.detectLast(bars);
        if (ib.detected) {
            // Требуем определённости тренда (по флагу)
            const bool trend_ok =
                !cfg_.require_trend_for_consolidation ||
                (ctx.dominant_trend != core::TrendDirection::Undefined);
            if (trend_ok && ctx.dominant_trend != core::TrendDirection::Undefined) {
                core::PatternSignal s;
                s.detected      = true;
                s.side          = (ctx.dominant_trend == core::TrendDirection::Bullish)
                                ? core::OrderSide::Buy
                                : core::OrderSide::Sell;
                s.type          = core::PatternType::Consolidation;
                s.trigger_price = bar.close;
                s.level         = ib.mother_high;   // верхняя граница сжатия
                s.suggested_stop = (s.side == core::OrderSide::Buy)
                                 ? ib.mother_low  - cfg_.pinbar_buy.stop_buffer_points * cfg_.point
                                 : ib.mother_high + cfg_.pinbar_sell.stop_buffer_points * cfg_.point;
                s.confidence    = ib.confidence;
                return s;
            }
        }
    }
    return best;   // пустой
}
} // namespace spartak::patterns