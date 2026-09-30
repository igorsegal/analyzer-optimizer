// =============================================================================
//  SPARTAK :: Spartak/pat_aggregate.mqh
//  Orchestrates pattern detectors. Returns first matched signal.
// =============================================================================
#ifndef SPARTAK_PAT_AGGREGATE_MQH
#define SPARTAK_PAT_AGGREGATE_MQH

#include <Spartak/contracts.mqh>
#include <Spartak/pat_geometry.mqh>
#include <Spartak/pat_consolidation.mqh>

struct SpkPatternCfg {
    SpkConsolidationCfg consolidation;
};

void pat_defaults(SpkPatternCfg &cfg)
{
    pat_consolidation_defaults(cfg.consolidation);
}

// Applies config skip flags.
bool pat_is_skipped(int pattern, int side, SpkConfig &cfg)
{
    if (pattern == SPK_PAT_CONSOLIDATION)
    {
        if (side == SPK_BUY  && cfg.skip_consolidation_buy)  return true;
        if (side == SPK_SELL && cfg.skip_consolidation_sell) return true;
    }
    if (pattern == SPK_PAT_FALSE_BREAKOUT && cfg.skip_false_breakout) return true;
    if (pattern == SPK_PAT_IMPULSE_BREAKOUT)
    {
        if (side == SPK_BUY  && cfg.skip_impulse_buy)  return true;
        if (side == SPK_SELL && cfg.skip_impulse_sell) return true;
    }
    return false;
}

// Try all detectors on current bar. Return true on first accepted signal.
bool pat_aggregate(string symbol, int tf, int shift,
                   SpkPatternCfg &pcfg, SpkConfig &cfg,
                   SpkSignal &out)
{
    SpkSignal s;

    if (pat_consolidation_detect(symbol, tf, shift, pcfg.consolidation, s))
    {
        if (!pat_is_skipped(s.pattern, s.side, cfg))
        {
            out = s;
            return true;
        }
    }

    return false;
}

#endif