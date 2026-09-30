// =============================================================================
//  SPARTAK :: Spartak/ctx_zones.mqh
//  PU (intermediate) / LU (local) zones from fractals.
// =============================================================================
#ifndef SPARTAK_CTX_ZONES_MQH
#define SPARTAK_CTX_ZONES_MQH

#include <Spartak/contracts.mqh>

#define SPK_LEVEL_LU 0
#define SPK_LEVEL_PU 1

struct SpkZone {
    int      type;
    datetime formation_time;
    double   price;
    double   top;
    double   bottom;
};

int ctx_zones_collect(string symbol, int tf, int radius, int max_zones,
                      SpkZone &out[])
{
    ArrayResize(out, 0);
    int lookback = 200;
    if (iBars(symbol, tf) < lookback) return 0;

    for (int i = radius; i < lookback; i++)
    {
        if (ArraySize(out) >= max_zones) break;

        double hi = iHigh(symbol, tf, i);
        double lo = iLow (symbol, tf, i);

        bool isHigh = true, isLow = true;
        for (int j = 1; j <= radius; j++)
        {
            if (iHigh(symbol, tf, i - j) >= hi) isHigh = false;
            if (iHigh(symbol, tf, i + j) >= hi) isHigh = false;
            if (iLow (symbol, tf, i - j) <= lo) isLow  = false;
            if (iLow (symbol, tf, i + j) <= lo) isLow  = false;
        }
        if (!isHigh && !isLow) continue;

        double point = MarketInfo(symbol, MODE_POINT);
        double offset_pu = 15.0 * point;
        double offset_lu = 10.0 * point;

        int n = ArraySize(out);
        ArrayResize(out, n + 1);

        if (isHigh)
        {
            out[n].type = SPK_LEVEL_PU;
            out[n].price = hi;
            out[n].top = hi + offset_pu;
            out[n].bottom = hi - offset_pu;
        }
        else
        {
            out[n].type = SPK_LEVEL_LU;
            out[n].price = lo;
            out[n].top = lo + offset_lu;
            out[n].bottom = lo - offset_lu;
        }
        out[n].formation_time = iTime(symbol, tf, i);
    }
    return ArraySize(out);
}

// Price is inside any active PU/LU zone?
bool ctx_zones_inside(SpkZone &zones[], double price)
{
    for (int i = 0; i < ArraySize(zones); i++)
        if (price >= zones[i].bottom && price <= zones[i].top)
            return true;
    return false;
}

#endif