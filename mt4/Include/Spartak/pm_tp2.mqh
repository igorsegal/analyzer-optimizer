// =============================================================================
//  SPARTAK :: Spartak/pm_tp2.mqh
//  TP2 hit detection for SELL position via current BID.
// =============================================================================
#ifndef SPARTAK_PM_TP2_MQH
#define SPARTAK_PM_TP2_MQH

#include <Spartak/contracts.mqh>

bool pm_tp2_hit(SpkPosition &p)
{
    if (!p.tp1_hit) return false;
    double bid = MarketInfo(p.symbol, MODE_BID);
    if (bid <= 0.0) return false;
    if (p.side == SPK_SELL) return (bid <= p.take_profit_2);
    if (p.side == SPK_BUY)  return (bid >= p.take_profit_2);
    return false;
}

#endif