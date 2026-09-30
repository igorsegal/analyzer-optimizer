// =============================================================================
//  SPARTAK :: Spartak/sig_collect.mqh
//  Collect all accepted signals at given shift across universe.
// =============================================================================
#ifndef SPARTAK_SIG_COLLECT_MQH
#define SPARTAK_SIG_COLLECT_MQH

#include <Spartak/contracts.mqh>
#include <Spartak/pat_aggregate.mqh>

int sig_collect(string &syms[], int n, int tf, int shift,
                SpkPatternCfg &pcfg, SpkConfig &cfg,
                SpkSignal &out[])
{
    ArrayResize(out, 0);
    for (int i = 0; i < n; i++)
    {
        SpkSignal s;
        if (pat_aggregate(syms[i], tf, shift, pcfg, cfg, s))
        {
            int k = ArraySize(out);
            if (k >= SPK_MAX_SIGNALS) break;
            ArrayResize(out, k + 1);
            out[k] = s;
        }
    }
    return ArraySize(out);
}

#endif