// =============================================================================
//  SPARTAK :: Spartak/pm_be.mqh
//  Break-even SL level (entry price).
// =============================================================================
#ifndef SPARTAK_PM_BE_MQH
#define SPARTAK_PM_BE_MQH

#include <Spartak/contracts.mqh>

double pm_be_level(SpkPosition &p)
{
    // Slight buffer into profit to cover spread
    double point = MarketInfo(p.symbol, MODE_POINT);
    double bid   = MarketInfo(p.symbol, MODE_BID);
    double ask   = MarketInfo(p.symbol, MODE_ASK);
    double spread = (ask > bid) ? (ask - bid) : (point * 10.0);

    if (p.side == SPK_SELL) return NormalizeDouble(p.entry_price - spread, (int)MarketInfo(p.symbol, MODE_DIGITS));
    if (p.side == SPK_BUY)  return NormalizeDouble(p.entry_price + spread, (int)MarketInfo(p.symbol, MODE_DIGITS));
    return 0.0;
}

#endif