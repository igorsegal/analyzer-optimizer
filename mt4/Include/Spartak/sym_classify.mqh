// =============================================================================
//  SPARTAK :: Spartak/sym_classify.mqh
//  Classify symbol name: FX / METAL / CRYPTO / UNKNOWN.
// =============================================================================
#ifndef SPARTAK_SYM_CLASSIFY_MQH
#define SPARTAK_SYM_CLASSIFY_MQH

#include <Spartak/contracts.mqh>

int sym_classify(string name)
{
    // Strip broker suffix after dot
    string base = name;
    int dot = StringFind(base, ".");
    if (dot > 0) base = StringSubstr(base, 0, dot);
    StringToUpper(base);

    // Metals
    if (StringFind(base, "XAU") == 0) return SPK_CAT_METAL;
    if (StringFind(base, "XAG") == 0) return SPK_CAT_METAL;
    if (StringFind(base, "XPT") == 0) return SPK_CAT_METAL;
    if (StringFind(base, "XPD") == 0) return SPK_CAT_METAL;

    // Crypto
    string crypto_bases[11] = {"BTC","ETH","SOL","XRP","ADA",
                              "DOGE","LTC","BCH","BNB","DOT","LINK"};
    for (int i = 0; i < ArraySize(crypto_bases); i++)
    {
        if (StringFind(base, crypto_bases[i]) == 0)
        {
            string tail = StringSubstr(base, StringLen(crypto_bases[i]));
            if (tail == "USD" || tail == "USDT") return SPK_CAT_CRYPTO;
        }
    }

    // Forex: 6 letters, both halves are known currencies
    if (StringLen(base) == 6)
    {
        string cur1 = StringSubstr(base, 0, 3);
        string cur2 = StringSubstr(base, 3, 3);
        string allowed[8] = {"USD","EUR","GBP","JPY",
                            "CHF","CAD","AUD","NZD"};
        bool ok1 = false, ok2 = false;
        for (int i = 0; i < ArraySize(allowed); i++)
        {
            if (cur1 == allowed[i]) ok1 = true;
            if (cur2 == allowed[i]) ok2 = true;
        }
        if (ok1 && ok2 && cur1 != cur2) return SPK_CAT_FOREX;
    }

    return SPK_CAT_UNKNOWN;
}

#endif