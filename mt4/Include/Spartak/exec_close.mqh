// =============================================================================
//  SPARTAK :: Spartak/exec_close.mqh
// =============================================================================
#ifndef SPARTAK_EXEC_CLOSE_MQH
#define SPARTAK_EXEC_CLOSE_MQH

#include <Spartak/contracts.mqh>
#include <Spartak/log.mqh>

bool exec_close(int ticket, double volume, SpkConfig &cfg)
{
    if (!OrderSelect(ticket, SELECT_BY_TICKET, MODE_TRADES))
    {
        log_write(SPK_LOG_WARN, "close: ticket not found " + IntegerToString(ticket));
        return false;
    }
    if (OrderType() != OP_BUY && OrderType() != OP_SELL) return false;

    string symbol = OrderSymbol();
    int dg = (int)MarketInfo(symbol, MODE_DIGITS);
    double price = (OrderType() == OP_BUY) ? MarketInfo(symbol, MODE_BID)
                                           : MarketInfo(symbol, MODE_ASK);
    if (price <= 0.0) return false;

    if (volume > OrderLots()) volume = OrderLots();
    if (volume <= 0.0) return false;

    bool ok = OrderClose(ticket, volume, price, cfg.slippage_points, clrYellow);
    if (!ok)
    {
        int err = GetLastError();
        log_write(SPK_LOG_ERROR,
                  "OrderClose fail " + symbol +
                  " ticket=" + IntegerToString(ticket) +
                  " vol=" + DoubleToString(volume, 2) +
                  " err=" + IntegerToString(err));
        return false;
    }

    log_write(SPK_LOG_INFO,
              "CLOSE " + symbol +
              " ticket=" + IntegerToString(ticket) +
              " vol=" + DoubleToString(volume, 2) +
              " price=" + DoubleToString(price, dg));
    return true;
}

#endif