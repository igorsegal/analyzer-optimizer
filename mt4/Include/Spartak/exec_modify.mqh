// =============================================================================
//  SPARTAK :: Spartak/exec_modify.mqh
// =============================================================================
#ifndef SPARTAK_EXEC_MODIFY_MQH
#define SPARTAK_EXEC_MODIFY_MQH

#include <Spartak/contracts.mqh>
#include <Spartak/log.mqh>

bool exec_modify_sl(int ticket, double new_sl, SpkConfig &cfg)
{
    if (!OrderSelect(ticket, SELECT_BY_TICKET, MODE_TRADES))
    {
        log_write(SPK_LOG_WARN, "modify: ticket not found " + IntegerToString(ticket));
        return false;
    }
    if (OrderType() != OP_BUY && OrderType() != OP_SELL) return false;

    string symbol = OrderSymbol();
    int dg = (int)MarketInfo(symbol, MODE_DIGITS);
    double sl = NormalizeDouble(new_sl, dg);
    if (sl <= 0.0) return false;

    double cur_sl = OrderStopLoss();
    if (MathAbs(cur_sl - sl) < MarketInfo(symbol, MODE_POINT) * 0.5) return false;

    double bid = MarketInfo(symbol, MODE_BID);
    double ask = MarketInfo(symbol, MODE_ASK);
    double stop_lvl = MarketInfo(symbol, MODE_STOPLEVEL) * MarketInfo(symbol, MODE_POINT);

    if (OrderType() == OP_BUY)
    {
        if (sl > bid - stop_lvl) return false;
    }
    else
    {
        if (sl < ask + stop_lvl) return false;
    }

    bool ok = OrderModify(ticket, OrderOpenPrice(), sl, OrderTakeProfit(), 0, clrGreen);
    if (!ok)
    {
        int err = GetLastError();
        log_write(SPK_LOG_ERROR,
                  "OrderModify fail " + symbol +
                  " ticket=" + IntegerToString(ticket) +
                  " new_sl=" + DoubleToString(sl, dg) +
                  " err=" + IntegerToString(err));
        return false;
    }

    log_write(SPK_LOG_INFO,
              "MODIFY_SL " + symbol +
              " ticket=" + IntegerToString(ticket) +
              " sl=" + DoubleToString(sl, dg));
    return true;
}

#endif