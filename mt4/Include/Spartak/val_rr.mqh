// =============================================================================
//  SPARTAK :: Spartak/val_rr.mqh
//  Compute TP1/TP2 from entry & stop, evaluate rr.
//  TP ratios fixed: TP1 = 1.5R, TP2 = 3.0R.
// =============================================================================
#ifndef SPARTAK_VAL_RR_MQH
#define SPARTAK_VAL_RR_MQH

#include <Spartak/contracts.mqh>

#define SPK_TP1_R  1.5
#define SPK_TP2_R  3.0

bool val_rr_eval(SpkSignal &s, double entry, double &sl, double &tp1, double &tp2, double &rr)
{
    sl  = s.stop;
    double risk = 0.0;
    if (s.side == SPK_BUY)  risk = entry - sl;
    if (s.side == SPK_SELL) risk = sl - entry;
    if (risk <= 0.0) return false;

    if (s.side == SPK_BUY)
    {
        tp1 = entry + risk * SPK_TP1_R;
        tp2 = entry + risk * SPK_TP2_R;
        rr  = (tp2 - entry) / risk;
    }
    else
    {
        tp1 = entry - risk * SPK_TP1_R;
        tp2 = entry - risk * SPK_TP2_R;
        rr  = (entry - tp2) / risk;
    }
    return (rr > 0.0);
}

#endif