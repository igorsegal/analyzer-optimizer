// =============================================================================
//  SPARTAK :: Spartak/pm_dispatch.mqh
//  Execute the action decided by pm_trigger.
// =============================================================================
#ifndef SPARTAK_PM_DISPATCH_MQH
#define SPARTAK_PM_DISPATCH_MQH

#include <Spartak/contracts.mqh>
#include <Spartak/log.mqh>
#include <Spartak/exec_close.mqh>
#include <Spartak/exec_modify.mqh>
#include <Spartak/pm_be.mqh>
#include <Spartak/pm_trail.mqh>

void pm_dispatch(SpkPosition &p, int ev, int tf, SpkConfig &cfg)
{
    if (ev == PM_EV_DEAD)
    {
        log_write(SPK_LOG_INFO, "position closed on server " + p.symbol +
                  " ticket=" + IntegerToString((long)p.ticket));
        p.closed = true;
        return;
    }

    if (ev == PM_EV_TP2)
    {
        double vol = OrderLots();
        if (exec_close(p.ticket, vol, cfg))
        {
            p.closed = true;
        }
        return;
    }

    if (ev == PM_EV_TP1)
    {
        double point = MarketInfo(p.symbol, MODE_POINT);
        double half = p.volume_open * cfg.tp1_close_fraction;
        double lot_step = MarketInfo(p.symbol, MODE_LOTSTEP);
        double min_lot  = MarketInfo(p.symbol, MODE_MINLOT);
        half = MathFloor(half / lot_step + 1e-9) * lot_step;
        half = MathFloor(half * 100.0 + 0.5) / 100.0;

        if (half >= min_lot && half < p.volume_open)
        {
            if (exec_close(p.ticket, half, cfg))
            {
                p.tp1_hit = true;
                p.volume_remain = OrderLots();
            }
        }
        else
        {
            // Too small for partial вЂ” treat as full close
            double vol = OrderLots();
            if (exec_close(p.ticket, vol, cfg))
            {
                p.closed = true;
                return;
            }
        }

        // Move SL to break-even
        double be = pm_be_level(p);
        if (be > 0.0 && exec_modify_sl(p.ticket, be, cfg))
        {
            p.stop_loss = be;
            p.be_moved  = true;
        }
        return;
    }

    if (ev == PM_EV_TRAIL)
    {
        double new_sl = pm_trail_level(p, tf, cfg);
        if (MathAbs(new_sl - p.stop_loss) > MarketInfo(p.symbol, MODE_POINT) * 0.5)
        {
            if (exec_modify_sl(p.ticket, new_sl, cfg))
            {
                p.stop_loss = new_sl;
            }
        }
        return;
    }
}

#endif