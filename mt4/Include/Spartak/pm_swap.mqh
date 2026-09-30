// =============================================================================
//  SPARTAK :: Spartak/pm_swap.mqh
//  Swap tracking placeholder. MT4 accrues swaps automatically on close.
// =============================================================================
#ifndef SPARTAK_PM_SWAP_MQH
#define SPARTAK_PM_SWAP_MQH

#include <Spartak/contracts.mqh>

// No-op in v1: MT4 handles swap accrual internally.
void pm_swap_apply(SpkPosition &p, SpkConfig &cfg) { }

#endif