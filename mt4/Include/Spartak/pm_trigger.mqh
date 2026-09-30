// =============================================================================
//  SPARTAK :: Spartak/pm_trigger.mqh
//  Decide what happened to a position at current tick.
//  Returns:
//    PM_EV_NONE    - nothing
//    PM_EV_TP1     - TP1 hit, need partial + BE
//    PM_EV_TP2     - TP2 hit, close remaining
//    PM_EV_TRAIL   - only SL needs to be moved (trail)
//    PM_EV_DEAD    - position closed on server (SL hit)
// =============================================================================
#ifndef SPARTAK_PM_TRIGGER_MQH
#define SPARTAK_PM_TRIGGER_MQH

#include <Spartak/contracts.mqh>
#include <Spartak/pm_sl.mqh>
#include <Spartak/pm_tp1.mqh>
#include <Spartak/pm_tp2.mqh>

#define PM_EV_NONE   0
#define PM_EV_TP1    1
#define PM_EV_TP2    2
#define PM_EV_TRAIL  3
#define PM_EV_DEAD   4

int pm_trigger(SpkPosition &p, int tf, SpkConfig &cfg)
{
    if (!pm_sl_alive(p.ticket)) return PM_EV_DEAD;

    // Sync remaining volume with server
    if (OrderSelect((int)p.ticket, SELECT_BY_TICKET, MODE_TRADES))
    {
        double lots = OrderLots();
        if (lots > 0.0 && lots < p.volume_remain - 1e-9)
        {
            p.volume_remain = lots;
            if (!p.tp1_hit) p.tp1_hit = true;
        }
    }

    if (pm_tp2_hit(p)) return PM_EV_TP2;

    if (!p.tp1_hit && pm_tp1_hit(p)) return PM_EV_TP1;

    if (p.tp1_hit && cfg.trailing_enabled) return PM_EV_TRAIL;

    return PM_EV_NONE;
}

#endif