// =============================================================================
//  SPARTAK :: SpartakCombine.mq4
//  Orchestrator. Only OnInit/OnTick/OnTimer/OnDeinit.
// =============================================================================
#property strict
#property copyright "SPARTAK"
#property version   "1.00"

#include <Spartak/contracts.mqh>
#include <Spartak/cfg_defaults.mqh>
#include <Spartak/cfg_load.mqh>
#include <Spartak/log.mqh>
#include <Spartak/sym_classify.mqh>
#include <Spartak/sym_universe.mqh>
#include <Spartak/sym_info.mqh>
#include <Spartak/bar_get.mqh>
#include <Spartak/acc_get.mqh>
#include <Spartak/ctx_trend.mqh>
#include <Spartak/ctx_struct.mqh>
#include <Spartak/ctx_zones.mqh>
#include <Spartak/pat_geometry.mqh>
#include <Spartak/pat_consolidation.mqh>
#include <Spartak/pat_aggregate.mqh>
#include <Spartak/sig_collect.mqh>
#include <Spartak/sig_rank.mqh>
#include <Spartak/val_spread.mqh>
#include <Spartak/val_rr.mqh>
#include <Spartak/val_margin.mqh>
#include <Spartak/order_size.mqh>
#include <Spartak/val_signal.mqh>
#include <Spartak/exec_open.mqh>
#include <Spartak/exec_close.mqh>
#include <Spartak/exec_modify.mqh>
#include <Spartak/pm_sl.mqh>
#include <Spartak/pm_tp1.mqh>
#include <Spartak/pm_tp2.mqh>
#include <Spartak/pm_be.mqh>
#include <Spartak/pm_trail.mqh>
#include <Spartak/pm_swap.mqh>
#include <Spartak/pm_trigger.mqh>
#include <Spartak/pm_dispatch.mqh>
#include <Spartak/tracker.mqh>

input bool RealTrading = false;   // true = send orders, false = log only
input int  TimerMs     = 1000;    // OnTimer period

SpkConfig     g_cfg;
SpkPatternCfg g_pcfg;
string        g_syms[];
SpkSymbol     g_sym_info[];
int           g_mt4_tf = PERIOD_H1;

SpkPosition   g_positions[];
datetime      g_last_bar_time = 0;

// -----------------------------------------------------------------------------
int OnInit()
{
    Print("SPARTAK Combine: init. v",
          SPK_VERSION_MAJOR, ".", SPK_VERSION_MINOR, ".", SPK_VERSION_PATCH);

    if (!cfg_load(g_cfg, "Spartak\\spartak.ini"))
        Print("SPARTAK: config not found, using defaults");

    log_configure(g_cfg.log_enabled, g_cfg.log_dir, g_cfg.log_level);
    g_mt4_tf = bar_tf_from_string(g_cfg.timeframe);
    pat_defaults(g_pcfg);

    int n = sym_universe_build(g_cfg, g_syms);
    ArrayResize(g_sym_info, n);
    for (int i = 0; i < n; i++)
        sym_info_fill(g_syms[i], g_sym_info[i]);

    int orphans = tracker_orphans(g_cfg.magic);
    if (orphans > 0)
    {
        Print("SPARTAK: WARNING ", orphans,
              " existing positions with magic=", g_cfg.magic,
              ". Manual intervention required. EA halted.");
        return(INIT_FAILED);
    }

    ArrayResize(g_positions, 0);

    Print("SPARTAK: universe=", n,
          "  RealTrading=", RealTrading,
          "  tf=", g_cfg.timeframe,
          "  magic=", g_cfg.magic);

    EventSetMillisecondTimer(TimerMs);
    log_write(SPK_LOG_INFO, "init ok");
    return(INIT_SUCCEEDED);
}

// -----------------------------------------------------------------------------
void OnDeinit(const int reason)
{
    EventKillTimer();
    log_write(SPK_LOG_INFO, "deinit reason=" + IntegerToString(reason));
    Print("SPARTAK Combine: deinit. Reason=", reason);
}

// -----------------------------------------------------------------------------
void scan_and_open()
{
    int n = ArraySize(g_syms);
    if (n == 0) return;

    SpkSignal sigs[];
    int cnt = sig_collect(g_syms, n, g_mt4_tf, 1, g_pcfg, g_cfg, sigs);
    if (cnt == 0) return;

    sig_rank(sigs);

    SpkAccount acc;
    acc_get(g_cfg, acc);

    for (int i = 0; i < cnt; i++)
    {
        int idx = -1;
        for (int j = 0; j < n; j++)
            if (g_syms[j] == sigs[i].symbol) { idx = j; break; }
        if (idx < 0) continue;

        SpkOrder o;
        if (!val_signal(sigs[i], g_sym_info[idx], g_cfg, acc, g_mt4_tf, o))
            continue;

        Print("SPARTAK APPROVED: ", o.symbol,
              " side=", o.side,
              " vol=", DoubleToString(o.volume, 2),
              " entry=", DoubleToString(o.entry_price, 5),
              " sl=", DoubleToString(o.stop_loss, 5),
              " tp1=", DoubleToString(o.take_profit_1, 5),
              " tp2=", DoubleToString(o.take_profit_2, 5),
              " rr=", DoubleToString(sigs[i].rr, 2));

        if (!RealTrading) continue;

        int ticket = exec_open(o, g_cfg);
        if (ticket == 0) continue;

        int k = ArraySize(g_positions);
        ArrayResize(g_positions, k + 1);
        SpkPosition p;
        p.symbol         = o.symbol;
        p.id             = ticket;
        p.ticket         = ticket;
        p.side           = o.side;
        p.entry_spread   = (int)MarketInfo(o.symbol, MODE_SPREAD);
        p.tp1_hit        = false;
        p.tp2_hit        = false;
        p.be_moved       = false;
        p.closed         = false;
        p.entry_price    = o.entry_price;
        p.volume_open    = o.volume;
        p.volume_remain  = o.volume;
        p.stop_loss      = o.stop_loss;
        p.take_profit_1  = o.take_profit_1;
        p.take_profit_2  = o.take_profit_2;
        p.realized_pnl   = 0.0;
        p.total_swap     = 0.0;
        p.opened_at      = TimeCurrent();
        p.last_swap_day  = 0;
        g_positions[k] = p;
    }
}

// -----------------------------------------------------------------------------
void manage_positions()
{
    int n = ArraySize(g_positions);
    if (n == 0) return;

    for (int i = 0; i < n; i++)
    {
        if (g_positions[i].closed) continue;
        int ev = pm_trigger(g_positions[i], g_mt4_tf, g_cfg);
        if (ev != PM_EV_NONE)
            pm_dispatch(g_positions[i], ev, g_mt4_tf, g_cfg);
    }

    // Compact array: drop closed positions
    int w = 0;
    for (int i = 0; i < n; i++)
    {
        if (!g_positions[i].closed)
        {
            g_positions[w] = g_positions[i];
            w++;
        }
    }
    ArrayResize(g_positions, w);
}

// -----------------------------------------------------------------------------
void OnTick()
{
    // New bar on primary chart в†’ look for signals
    datetime t = iTime(Symbol(), g_mt4_tf, 0);
    if (t != g_last_bar_time)
    {
        g_last_bar_time = t;
        scan_and_open();
    }
}

// -----------------------------------------------------------------------------
void OnTimer()
{
    manage_positions();
}