// =============================================================================
//  SPARTAK :: Spartak/pat_geometry.mqh
//  Candle geometry: body, wicks, ratios.
// =============================================================================
#ifndef SPARTAK_PAT_GEOMETRY_MQH
#define SPARTAK_PAT_GEOMETRY_MQH

#include <Spartak/contracts.mqh>

struct SpkCandle {
    double open;
    double high;
    double low;
    double close;
    double body;
    double range;
    double upper_wick;
    double lower_wick;
    double body_ratio;
    double upper_ratio;
    double lower_ratio;
    double close_pos;
    bool   bull;
    bool   bear;
    bool   doji;
};

void pat_candle_analyze(const SpkBar &b, SpkCandle &out)
{
    out.open       = b.open;
    out.high       = b.high;
    out.low        = b.low;
    out.close      = b.close;
    out.range      = b.high - b.low;
    out.body       = MathAbs(b.close - b.open);
    out.upper_wick = b.high - MathMax(b.open, b.close);
    out.lower_wick = MathMin(b.open, b.close) - b.low;

    if (out.range > 0.0)
    {
        out.body_ratio  = out.body       / out.range;
        out.upper_ratio = out.upper_wick / out.range;
        out.lower_ratio = out.lower_wick / out.range;
        out.close_pos   = (b.close - b.low) / out.range;
    }
    else
    {
        out.body_ratio  = 0.0;
        out.upper_ratio = 0.0;
        out.lower_ratio = 0.0;
        out.close_pos   = 0.5;
    }

    out.bull = (b.close > b.open);
    out.bear = (b.close < b.open);
    out.doji = (out.body_ratio < 0.05);
}

#endif