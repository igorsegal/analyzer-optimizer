// =============================================================================
//  SPARTAK :: Spartak/pat_consolidation.mqh
//  Consolidation (Inside Bar) detector. Produces SELL signals only.
//  Logic mirror of C++ InsideBarDetector + direction filter.
// =============================================================================
#ifndef SPARTAK_PAT_CONSOLIDATION_MQH
#define SPARTAK_PAT_CONSOLIDATION_MQH

#include <Spartak/contracts.mqh>
#include <Spartak/pat_geometry.mqh>
#include <Spartak/ctx_trend.mqh>
#include <Spartak/ctx_struct.mqh>

struct SpkConsolidationCfg {
    double min_range_ratio;     // max current/mother ratio to accept
    double stop_buffer_points;  // extra pts beyond mother low for SL
    bool   require_trend;       // require defined trend
    bool   sell_only;           // only emit SELL signals
};

void pat_consolidation_defaults(SpkConsolidationCfg &cfg)
{
    cfg.min_range_ratio    = 0.5;
    cfg.stop_buffer_points = 30.0;
    cfg.require_trend      = true;
    cfg.sell_only          = true;
}

// mother = bar at shift+1, current = bar at shift
bool pat_consolidation_detect(string symbol, int tf, int shift,
                              SpkConsolidationCfg &cfg,
                              SpkSignal &out)
{
    out.symbol    = symbol;
    out.side      = -1;
    out.pattern   = SPK_PAT_NONE;
    out.confidence = 0.0;
    out.trigger   = 0.0;
    out.level     = 0.0;
    out.stop      = 0.0;

    SpkBar curr, moth;
    if (!bar_get(symbol, tf, shift,     curr)) return false;
    if (!bar_get(symbol, tf, shift + 1, moth)) return false;

    // Inside bar: curr range inside mother range
    if (curr.high > moth.high) return false;
    if (curr.low  < moth.low)  return false;

    double mother_range = moth.high - moth.low;
    if (mother_range <= 0.0) return false;
    double curr_range = curr.high - curr.low;
    double ratio = curr_range / mother_range;
    if (ratio > cfg.min_range_ratio) return false;

    // Trend filter
    if (cfg.require_trend)
    {
        SpkTrend tr;
        ctx_trend_calc(symbol, tr);
        if (tr.dominant == SPK_TREND_UNDEF) return false;
        // SELL only вЂ” require no bullish dominant trend
        if (cfg.sell_only && tr.dominant == SPK_TREND_BULL) return false;
    }

    // Build signal: SELL
    double point = MarketInfo(symbol, MODE_POINT);
    out.symbol      = symbol;
    out.bar_time    = curr.time;
    out.side        = SPK_SELL;
    out.pattern     = SPK_PAT_CONSOLIDATION;
    out.trigger     = curr.low;
    out.level       = moth.low;
    out.stop        = moth.low + cfg.stop_buffer_points * point;
    out.confidence  = 1.0 - ratio;
    return true;
}

#endif