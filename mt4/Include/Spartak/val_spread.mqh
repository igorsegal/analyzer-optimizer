// =============================================================================
//  SPARTAK :: Spartak/val_spread.mqh
// =============================================================================
#ifndef SPARTAK_VAL_SPREAD_MQH
#define SPARTAK_VAL_SPREAD_MQH

#include <Spartak/contracts.mqh>
#include <Spartak/log.mqh>
#include <Spartak/sym_classify.mqh>

int val_spread_limit(string symbol, SpkConfig &cfg)
{
    int cat = sym_classify(symbol);
    if (cat == SPK_CAT_METAL)  return cfg.max_spread_metal;
    if (cat == SPK_CAT_CRYPTO) return cfg.max_spread_crypto;
    return cfg.max_spread_fx;
}

bool val_spread_ok(string symbol, SpkConfig &cfg)
{
    int sp = (int)MarketInfo(symbol, MODE_SPREAD);
    int limit = (int)(val_spread_limit(symbol, cfg) * cfg.spread_mult);
    if (sp > limit)
    {
        log_write(SPK_LOG_DEBUG, "spread reject " + symbol +
                  " sp=" + IntegerToString(sp) +
                  " limit=" + IntegerToString(limit));
        return false;
    }
    return true;
}

#endif