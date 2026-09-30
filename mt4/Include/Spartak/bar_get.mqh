// =============================================================================
//  SPARTAK :: Spartak/bar_get.mqh
//  Read a bar from MT4 into SpkBar.
// =============================================================================
#ifndef SPARTAK_BAR_GET_MQH
#define SPARTAK_BAR_GET_MQH

#include <Spartak/contracts.mqh>

int bar_tf_from_string(string tf)
{
    if (tf == "M1")  return PERIOD_M1;
    if (tf == "M5")  return PERIOD_M5;
    if (tf == "M15") return PERIOD_M15;
    if (tf == "M30") return PERIOD_M30;
    if (tf == "H1")  return PERIOD_H1;
    if (tf == "H4")  return PERIOD_H4;
    if (tf == "D1")  return PERIOD_D1;
    return PERIOD_H1;
}

bool bar_get(string symbol, int mt4_tf, int shift, SpkBar &out)
{
    datetime t = iTime(symbol, mt4_tf, shift);
    if (t == 0) return false;

    out.time   = t;
    out.open   = iOpen(symbol, mt4_tf, shift);
    out.high   = iHigh(symbol, mt4_tf, shift);
    out.low    = iLow(symbol, mt4_tf, shift);
    out.close  = iClose(symbol, mt4_tf, shift);
    out.volume = (long)iVolume(symbol, mt4_tf, shift);
    out.spread = (int)MarketInfo(symbol, MODE_SPREAD);
    return true;
}

#endif