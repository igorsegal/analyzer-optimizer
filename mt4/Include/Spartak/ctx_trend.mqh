// =============================================================================
//  SPARTAK :: Spartak/ctx_trend.mqh
// =============================================================================
#ifndef SPARTAK_CTX_TREND_MQH
#define SPARTAK_CTX_TREND_MQH

#include <Spartak/contracts.mqh>

#define SPK_TREND_UNDEF   0
#define SPK_TREND_BULL    1
#define SPK_TREND_BEAR    2

struct SpkTrend {
    int  daily;
    int  h1;
    int  dominant;
    bool conflict;
};

int ctx_trend_of(string symbol, int tf, int lookback)
{
    if (iBars(symbol, tf) < lookback + 2) return SPK_TREND_UNDEF;
    double c_now = iClose(symbol, tf, 0);
    double c_ref = iClose(symbol, tf, lookback);
    if (c_now <= 0.0 || c_ref <= 0.0) return SPK_TREND_UNDEF;
    double diff = c_now - c_ref;
    double noise = 0.0001 * c_ref;
    if (diff >  noise) return SPK_TREND_BULL;
    if (diff < -noise) return SPK_TREND_BEAR;
    return SPK_TREND_UNDEF;
}

void ctx_trend_calc(string symbol, SpkTrend &out)
{
    out.daily  = ctx_trend_of(symbol, PERIOD_D1, 3);
    out.h1     = ctx_trend_of(symbol, PERIOD_H1, 24);
    out.conflict = false;
    if (out.daily == out.h1 && out.daily != SPK_TREND_UNDEF)
        out.dominant = out.daily;
    else if (out.daily != SPK_TREND_UNDEF && out.h1 == SPK_TREND_UNDEF)
        out.dominant = out.daily;
    else if (out.daily == SPK_TREND_UNDEF && out.h1 != SPK_TREND_UNDEF)
        out.dominant = out.h1;
    else if (out.daily != SPK_TREND_UNDEF && out.h1 != SPK_TREND_UNDEF
             && out.daily != out.h1)
    {
        out.dominant = SPK_TREND_UNDEF;
        out.conflict = true;
    }
    else
        out.dominant = SPK_TREND_UNDEF;
}

#endif