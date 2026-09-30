// =============================================================================
//  SPARTAK :: Spartak/pm_trail.mqh
//  Trailing SL for SELL after TP1: follows the highest high of last closed bar.
//  Moves SL only in the profitable direction (down for SELL).
// =============================================================================
#ifndef SPARTAK_PM_TRAIL_MQH
#define SPARTAK_PM_TRAIL_MQH

#include <Spartak/contracts.mqh>

double pm_trail_level(SpkPosition &p, int tf, SpkConfig &cfg)
{
    double point = MarketInfo(p.symbol, MODE_POINT);
    double hh = iHigh(p.symbol, tf, 1);
    if (hh <= 0.0) return p.stop_loss;

    double dist = cfg.trailing_distance_points * point;
    double new_sl = hh - dist;

    // For SELL: only move SL down (never up)
    if (p.side == SPK_SELL && new_sl < p.stop_loss) return new_sl;
    if (p.side == SPK_BUY  && new_sl > p.stop_loss) return new_sl;
    return p.stop_loss;
}

#endif