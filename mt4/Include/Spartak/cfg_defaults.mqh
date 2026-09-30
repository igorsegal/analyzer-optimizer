// =============================================================================
//  SPARTAK :: Spartak/cfg_defaults.mqh
// =============================================================================
#ifndef SPARTAK_CFG_DEFAULTS_MQH
#define SPARTAK_CFG_DEFAULTS_MQH

#include <Spartak/contracts.mqh>

void cfg_defaults(SpkConfig &cfg)
{
    cfg.balance          = 10000.0;
    cfg.risk_percent     = 0.45;
    cfg.compound         = false;
    cfg.min_margin       = 200.0;
    cfg.min_rr           = 1.34;
    cfg.max_positions    = 0;

    cfg.skip_false_breakout     = true;
    cfg.skip_impulse_buy        = true;
    cfg.skip_impulse_sell       = true;
    cfg.skip_consolidation_buy  = true;
    cfg.skip_consolidation_sell = false;

    cfg.fx_major   = true;
    cfg.fx_cross   = true;
    cfg.metals_on  = true;
    cfg.crypto_on  = true;
    cfg.jpy_pairs  = false;

    cfg.timeframe    = "H1";
    cfg.history_bars = 500;

    cfg.session_enabled          = false;
    cfg.session_start            = 0;
    cfg.session_end              = 0;
    cfg.skip_friday_after_hour   = 20;
    cfg.skip_monday_before_hour  = 2;

    cfg.max_spread_fx     = 40;
    cfg.max_spread_metal  = 50;
    cfg.max_spread_crypto = 200;
    cfg.slippage_points   = 30;
    cfg.spread_mult       = 1.0;
    cfg.commission_mult   = 1.0;

    cfg.trailing_enabled            = true;
    cfg.trailing_distance_points    = 50;
    cfg.trailing_activation_points  = 100;
    cfg.trail_only_after_tp1        = true;
    cfg.be_enabled                  = true;
    cfg.tp1_close_fraction          = 0.5;
    cfg.tp2_full_close              = true;
    cfg.swap_enabled                = true;

    cfg.emergency_enabled        = false;
    cfg.min_margin_level_pct     = 100.0;
    cfg.emergency_drawdown_pct   = 0.0;

    cfg.log_enabled = true;
    cfg.log_dir     = "Spartak";
    cfg.log_level   = SPK_LOG_INFO;
    cfg.trades_csv  = true;

    cfg.magic         = 20260928;
    cfg.allow_weekend = false;
}

#endif