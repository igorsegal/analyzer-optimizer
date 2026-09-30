// =============================================================================
//  SPARTAK :: Spartak/ctx_struct.mqh
//  HH/HL structure over lookback bars.
// =============================================================================
#ifndef SPARTAK_CTX_STRUCT_MQH
#define SPARTAK_CTX_STRUCT_MQH

#include <Spartak/contracts.mqh>

#define SPK_STRUCT_UNDEF 0
#define SPK_STRUCT_HHHL  1
#define SPK_STRUCT_LHLL  2

// Find last two swing highs/lows with given radius, over lookback bars.
bool ctx_swing_highs(string sym, int tf, int radius, int lookback,
                     double &h1, double &h2)
{
    h1 = 0.0; h2 = 0.0;
    int found = 0;
    for (int i = radius; i < lookback; i++)
    {
        double c = iHigh(sym, tf, i);
        bool ok = true;
        for (int j = 1; j <= radius; j++)
        {
            if (iHigh(sym, tf, i - j) >= c) { ok = false; break; }
            if (iHigh(sym, tf, i + j) >= c) { ok = false; break; }
        }
        if (ok)
        {
            if (found == 0) h1 = c;
            else if (found == 1) { h2 = c; break; }
            found++;
        }
    }
    return (found >= 2);
}

bool ctx_swing_lows(string sym, int tf, int radius, int lookback,
                    double &l1, double &l2)
{
    l1 = 0.0; l2 = 0.0;
    int found = 0;
    for (int i = radius; i < lookback; i++)
    {
        double c = iLow(sym, tf, i);
        bool ok = true;
        for (int j = 1; j <= radius; j++)
        {
            if (iLow(sym, tf, i - j) <= c) { ok = false; break; }
            if (iLow(sym, tf, i + j) <= c) { ok = false; break; }
        }
        if (ok)
        {
            if (found == 0) l1 = c;
            else if (found == 1) { l2 = c; break; }
            found++;
        }
    }
    return (found >= 2);
}

int ctx_struct_calc(string symbol, int tf)
{
    double h1, h2, l1, l2;
    bool okH = ctx_swing_highs(symbol, tf, 2, 60, h1, h2);
    bool okL = ctx_swing_lows (symbol, tf, 2, 60, l1, l2);
    if (!okH || !okL) return SPK_STRUCT_UNDEF;

    bool hh = (h1 > h2);
    bool hl = (l1 > l2);
    if (hh && hl) return SPK_STRUCT_HHHL;
    if (!hh && !hl) return SPK_STRUCT_LHLL;
    return SPK_STRUCT_UNDEF;
}

#endif