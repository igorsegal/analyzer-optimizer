// =============================================================================
//  SPARTAK :: Spartak/pm_tp1.mqh
//  TP1 hit detection for SELL position via current BID.
// =============================================================================
#ifndef SPARTAK_PM_TP1_MQH
#define SPARTAK_PM_TP1_MQH

#include <Spartak/contracts.mqh>

bool pm_tp1_hit(SpkPosition &p)
{
    if (p.tp1_hit) return false;
    double bid = MarketInfo(p.symbol, MODE_BID);
    if (bid <= 0.0) return false;
    if (p.side == SPK_SELL) return (bid <= p.take_profit_1);
    if (p.side == SPK_BUY)  return (bid >= p.take_profit_1);
    return false;
}

#endif