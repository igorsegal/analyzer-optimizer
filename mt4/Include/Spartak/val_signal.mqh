// =============================================================================
//  SPARTAK :: Spartak/val_signal.mqh
//  Orchestrator: spread в†’ rr/tps в†’ size в†’ margin. Fills SpkOrder.
// =============================================================================
#ifndef SPARTAK_VAL_SIGNAL_MQH
#define SPARTAK_VAL_SIGNAL_MQH

#include <Spartak/contracts.mqh>
#include <Spartak/log.mqh>
#include <Spartak/val_spread.mqh>
#include <Spartak/val_rr.mqh>
#include <Spartak/val_margin.mqh>
#include <Spartak/order_size.mqh>

// Returns true on approved order, false otherwise.
bool val_signal(SpkSignal &s, SpkSymbol &info, SpkConfig &cfg,
                SpkAccount &acc, int tf, SpkOrder &out)
{
    out.symbol = s.symbol;
    out.side   = s.side;
    out.magic  = cfg.magic;
    out.volume = 0.0;

    if (!val_spread_ok(s.symbol, cfg)) return false;

    // Entry price = market now
    double entry = (s.side == SPK_BUY) ? MarketInfo(s.symbol, MODE_ASK)
                                       : MarketInfo(s.symbol, MODE_BID);
    if (entry <= 0.0) return false;

    // Step back: recompute stop distance relative to actual entry.
    // Signal stop is a level; keep absolute.
    double sl  = s.stop;
    double tp1 = 0.0, tp2 = 0.0, rr = 0.0;
    if (!val_rr_eval(s, entry, sl, tp1, tp2, rr)) return false;
    if (rr < cfg.min_rr) return false;
    s.rr = rr;

    // Volume sizing
    double sizing_balance = cfg.compound ? acc.balance : cfg.balance;
    double comm = info.commission_per_lot * cfg.commission_mult;
    double vol = order_size_calc(sizing_balance, cfg.risk_percent,
                                 entry, sl,
                                 info.contract_size, comm,
                                 info.min_lot, info.max_lot, info.lot_step);
    if (vol <= 0.0) return false;

    if (!val_margin_ok(s.symbol, vol, cfg, acc)) return false;

    out.entry_price   = entry;
    out.stop_loss     = sl;
    out.take_profit_1 = tp1;
    out.take_profit_2 = tp2;
    out.volume        = vol;
    out.risk_usd      = MathAbs(entry - sl) * vol * info.contract_size;
    out.comment       = "SPK:CONS";
    return true;
}

#endif