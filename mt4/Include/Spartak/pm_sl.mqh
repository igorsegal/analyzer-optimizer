// =============================================================================
//  SPARTAK :: Spartak/pm_sl.mqh
// =============================================================================
#ifndef SPARTAK_PM_SL_MQH
#define SPARTAK_PM_SL_MQH

#include <Spartak/contracts.mqh>

bool pm_sl_alive(int ticket)
{
    return OrderSelect(ticket, SELECT_BY_TICKET, MODE_TRADES);
}

#endif