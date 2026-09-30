// =============================================================================
//  SPARTAK :: spread_scan.mq4
//  Script. Prints avg/median spread for all Market Watch symbols
//  (or selected ones) over the last N H1 bars.
//  Output goes to Experts log.
// =============================================================================
#property strict
#property script_show_inputs

input int BarsBack = 500;  // how many H1 bars to average

void OnStart()
{
    Print("=== SPARTAK spread_scan ===");
    Print("BarsBack=", BarsBack);

    int total = SymbolsTotal(true);
    Print("MarketWatch symbols: ", total);
    Print("");
    Print(StringFormat("%-14s %-8s %8s %8s %8s %8s",
          "symbol", "category", "min", "avg", "median", "max"));

    for (int i = 0; i < total; i++)
    {
        string sym = SymbolName(i, true);
        int bars = MathMin(BarsBack, iBars(sym, PERIOD_H1));
        if (bars < 10) continue;

        int sp[];
        ArrayResize(sp, bars);
        for (int b = 0; b < bars; b++)
            sp[b] = (int)iSpread(sym, PERIOD_H1, b);

        // sort for median
        for (int a = 0; a < bars - 1; a++)
            for (int c = 0; c < bars - 1 - a; c++)
                if (sp[c] > sp[c + 1])
                {
                    int t = sp[c]; sp[c] = sp[c + 1]; sp[c + 1] = t;
                }

        int sum = 0, mn = sp[0], mx = sp[bars - 1];
        for (int b2 = 0; b2 < bars; b2++) sum += sp[b2];
        double avg = (double)sum / bars;
        double med = sp[bars / 2];

        Print(StringFormat("%-14s %-8d %8d %8.1f %8.0f %8d",
              sym, 0, mn, avg, med, mx));
    }
    Print("=== done ===");
}