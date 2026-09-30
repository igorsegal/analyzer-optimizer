// =============================================================================
//  SPARTAK :: Spartak/exec_open.mqh
// =============================================================================
#ifndef SPARTAK_EXEC_OPEN_MQH
#define SPARTAK_EXEC_OPEN_MQH

#include <Spartak/contracts.mqh>
#include <Spartak/log.mqh>

bool exec_has_position(string symbol, int magic)
{
    int total = OrdersTotal();
    for (int i = 0; i < total; i++)
    {
        if (!OrderSelect(i, SELECT_BY_POS, MODE_TRADES)) continue;
        if (OrderSymbol() != symbol) continue;
        if (OrderMagicNumber() != magic) continue;
        if (OrderType() == OP_BUY || OrderType() == OP_SELL) return true;
    }
    return false;
}

int exec_open(SpkOrder &o, SpkConfig &cfg)
{
    if (exec_has_position(o.symbol, cfg.magic))
    {
        log_write(SPK_LOG_DEBUG, "skip: position exists " + o.symbol);
        return 0;
    }

    double price = (o.side == SPK_BUY) ? MarketInfo(o.symbol, MODE_ASK)
                                       : MarketInfo(o.symbol, MODE_BID);
    if (price <= 0.0) return 0;

    int cmd  = (o.side == SPK_BUY) ? OP_BUY : OP_SELL;
    int slip = cfg.slippage_points;
    int dg   = (int)MarketInfo(o.symbol, MODE_DIGITS);

    double sl = NormalizeDouble(o.stop_loss, dg);

    string cmt = o.comment;
    if (StringLen(cmt) == 0) cmt = "SPK";

    int ticket = OrderSend(o.symbol, cmd, o.volume, price,
                           slip, sl, 0.0, cmt, cfg.magic, 0,
                           (o.side == SPK_BUY) ? clrBlue : clrRed);
    if (ticket <= 0)
    {
        int err = GetLastError();
        log_write(SPK_LOG_ERROR,
                  "OrderSend fail " + o.symbol +
                  " err=" + IntegerToString(err) +
                  " vol=" + DoubleToString(o.volume, 2) +
                  " price=" + DoubleToString(price, dg));
        return 0;
    }

    log_write(SPK_LOG_INFO,
              "OPEN " + o.symbol +
              " ticket=" + IntegerToString(ticket) +
              " side=" + IntegerToString(o.side) +
              " vol=" + DoubleToString(o.volume, 2) +
              " price=" + DoubleToString(price, dg) +
              " sl=" + DoubleToString(sl, dg));
    return ticket;
}

#endif