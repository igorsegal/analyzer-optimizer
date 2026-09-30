// =============================================================================
//  SPARTAK :: Spartak/val_margin.mqh
//  Margin-level check for a prospective order.
// =============================================================================
#ifndef SPARTAK_VAL_MARGIN_MQH
#define SPARTAK_VAL_MARGIN_MQH

#include <Spartak/contracts.mqh>

// Returns true if projected margin level stays above cfg.min_margin.
bool val_margin_ok(string symbol, double volume, SpkConfig &cfg, SpkAccount &acc)
{
    if (acc.equity <= 0.0) return false;
    double req_margin = MarketInfo(symbol, MODE_MARGINREQUIRED) * volume;
    double new_used   = acc.margin_used + req_margin;
    if (new_used <= 0.0) return true;
    double new_level  = (acc.equity / new_used) * 100.0;
    if (new_level < cfg.min_margin)
    {
        log_write(SPK_LOG_DEBUG, "margin reject " + symbol +
                  " new_level=" + DoubleToString(new_level, 2));
        return false;
    }
    return true;
}

#endif