// =============================================================================
//  SPARTAK :: Spartak/tracker.mqh
//  Count existing positions with our magic at OnInit.
// =============================================================================
#ifndef SPARTAK_TRACKER_MQH
#define SPARTAK_TRACKER_MQH

#include <Spartak/contracts.mqh>

int tracker_orphans(int magic)
{
    int found = 0;
    int total = OrdersTotal();
    for (int i = 0; i < total; i++)
    {
        if (!OrderSelect(i, SELECT_BY_POS, MODE_TRADES)) continue;
        if (OrderMagicNumber() != magic) continue;
        int t = OrderType();
        if (t == OP_BUY || t == OP_SELL) found++;
    }
    return found;
}

#endif