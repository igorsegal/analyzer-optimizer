// =============================================================================
//  SPARTAK :: Spartak/sym_universe.mqh
//  Build trade universe from Market Watch filtered by config.
// =============================================================================
#ifndef SPARTAK_SYM_UNIVERSE_MQH
#define SPARTAK_SYM_UNIVERSE_MQH

#include <Spartak/contracts.mqh>
#include <Spartak/sym_classify.mqh>

bool sym_is_jpy(string name)
{
    string base = name;
    int dot = StringFind(base, ".");
    if (dot > 0) base = StringSubstr(base, 0, dot);
    StringToUpper(base);
    return (StringFind(base, "JPY") >= 0);
}

int sym_universe_build(SpkConfig &cfg, string &syms[])
{
    ArrayResize(syms, 0);
    int total = SymbolsTotal(true);
    for (int i = 0; i < total; i++)
    {
        string name = SymbolName(i, true);
        int cat = sym_classify(name);
        if (cat == SPK_CAT_UNKNOWN) continue;

        if (cat == SPK_CAT_METAL  && !cfg.metals_on) continue;
        if (cat == SPK_CAT_CRYPTO && !cfg.crypto_on) continue;

        if (cat == SPK_CAT_FOREX)
        {
            if (sym_is_jpy(name) && !cfg.jpy_pairs) continue;
            bool is_major = (StringFind(name, "USD") >= 0);
            if (is_major  && !cfg.fx_major) continue;
            if (!is_major && !cfg.fx_cross) continue;
        }

        int n = ArraySize(syms);
        if (n >= SPK_MAX_SYMBOLS) break;
        ArrayResize(syms, n + 1);
        syms[n] = name;
    }
    return ArraySize(syms);
}

#endif