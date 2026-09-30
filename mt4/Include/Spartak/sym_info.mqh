// =============================================================================
//  SPARTAK :: Spartak/sym_info.mqh
//  Fill SpkSymbol from MarketInfo.
// =============================================================================
#ifndef SPARTAK_SYM_INFO_MQH
#define SPARTAK_SYM_INFO_MQH

#include <Spartak/contracts.mqh>
#include <Spartak/sym_classify.mqh>

bool sym_info_fill(string name, SpkSymbol &out)
{
    double point = MarketInfo(name, MODE_POINT);
    if (point <= 0.0) return false;

    out.name             = name;
    out.category         = sym_classify(name);
    out.digits           = (int)MarketInfo(name, MODE_DIGITS);
    out.point            = point;
    out.tick_value       = MarketInfo(name, MODE_TICKVALUE);
    out.tick_size        = MarketInfo(name, MODE_TICKSIZE);
    out.contract_size    = MarketInfo(name, MODE_LOTSIZE);
    out.min_lot          = MarketInfo(name, MODE_MINLOT);
    out.max_lot          = MarketInfo(name, MODE_MAXLOT);
    out.lot_step         = MarketInfo(name, MODE_LOTSTEP);
    out.stop_level_pts   = (int)MarketInfo(name, MODE_STOPLEVEL);
    out.spread_typical   = (int)MarketInfo(name, MODE_SPREAD);
    out.swap_long        = MarketInfo(name, MODE_SWAPLONG);
    out.swap_short       = MarketInfo(name, MODE_SWAPSHORT);
    out.commission_per_lot = 5.0;
    out.leverage         = (double)AccountLeverage();
    out.trade_mode       = 3;

    // base / quote
    string base = name;
    int dot = StringFind(base, ".");
    if (dot > 0) base = StringSubstr(base, 0, dot);
    if (StringLen(base) == 6)
    {
        out.base  = StringSubstr(base, 0, 3);
        out.quote = StringSubstr(base, 3, 3);
    }
    else
    {
        out.base  = "";
        out.quote = "";
    }
    return true;
}

#endif