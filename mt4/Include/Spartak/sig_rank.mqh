// =============================================================================
//  SPARTAK :: Spartak/sig_rank.mqh
//  Sort signals by rr descending (bubble вЂ” small N).
// =============================================================================
#ifndef SPARTAK_SIG_RANK_MQH
#define SPARTAK_SIG_RANK_MQH

#include <Spartak/contracts.mqh>

void sig_rank(SpkSignal &arr[])
{
    int n = ArraySize(arr);
    for (int i = 0; i < n - 1; i++)
    {
        for (int j = 0; j < n - 1 - i; j++)
        {
            if (arr[j].rr < arr[j + 1].rr)
            {
                SpkSignal t = arr[j];
                arr[j]     = arr[j + 1];
                arr[j + 1] = t;
            }
        }
    }
}

#endif