// =============================================================================
//  SPARTAK :: Spartak/order_size.mqh
//  Lot = risk_money / (stop_dist * contract_size), rounded down to lot_step.
//  Iterative commission correction, 3 passes.
// =============================================================================
#ifndef SPARTAK_ORDER_SIZE_MQH
#define SPARTAK_ORDER_SIZE_MQH

#include <Spartak/contracts.mqh>

double order_size_calc(double balance, double risk_percent,
                       double entry, double stop,
                       double contract_size, double commission_per_lot,
                       double min_lot, double max_lot, double lot_step)
{
    double stop_dist = MathAbs(entry - stop);
    if (stop_dist <= 0.0) return 0.0;

    double risk_total = balance * risk_percent / 100.0;
    if (risk_total <= 0.0) return 0.0;

    double lot = risk_total / (stop_dist * contract_size);
    for (int i = 0; i < 3; i++)
    {
        double comm = lot * commission_per_lot;
        double net  = risk_total - comm;
        if (net <= 0.0) return 0.0;
        lot = net / (stop_dist * contract_size);
    }

    double steps = MathFloor(lot / lot_step + 1e-9);
    lot = steps * lot_step;
    lot = MathFloor(lot * 100.0 + 0.5) / 100.0;

    if (lot < min_lot) return 0.0;
    if (lot > max_lot) lot = max_lot;
    return lot;
}

#endif