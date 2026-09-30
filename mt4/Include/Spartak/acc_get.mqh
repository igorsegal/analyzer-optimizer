// =============================================================================
//  SPARTAK :: Spartak/acc_get.mqh
//  Snapshot of trading account.
// =============================================================================
#ifndef SPARTAK_ACC_GET_MQH
#define SPARTAK_ACC_GET_MQH

#include <Spartak/contracts.mqh>

void acc_get(SpkConfig &cfg, SpkAccount &out)
{
    out.balance                 = AccountBalance();
    out.equity                  = AccountEquity();
    out.free_margin             = AccountFreeMargin();
    out.margin_used             = AccountMargin();
    out.leverage                = (double)AccountLeverage();
    out.min_margin_level_pct    = cfg.min_margin_level_pct;
    out.target_margin_level_pct = cfg.min_margin;

    if (out.margin_used > 0.0)
        out.margin_level_pct = (out.equity / out.margin_used) * 100.0;
    else
        out.margin_level_pct = 1000000.0;
}

#endif