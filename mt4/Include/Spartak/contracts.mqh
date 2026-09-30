// =============================================================================
//  SPARTAK :: Spartak/contracts.mqh
// =============================================================================
#ifndef SPARTAK_CONTRACTS_MQH
#define SPARTAK_CONTRACTS_MQH

#define SPK_VERSION_MAJOR   1
#define SPK_VERSION_MINOR   0
#define SPK_VERSION_PATCH   3

#define SPK_OK                     0
#define SPK_ERR_ABI_MISMATCH      -1
#define SPK_ERR_NOT_INITIALIZED   -2
#define SPK_ERR_UNKNOWN_SYMBOL    -3
#define SPK_ERR_BUFFER_TOO_SMALL  -4
#define SPK_ERR_INVALID_ARG       -5
#define SPK_ERR_MARGIN_LIMIT      -6
#define SPK_ERR_VALIDATION_FAILED -7
#define SPK_ERR_DUPLICATE_POS     -8
#define SPK_ERR_INTERNAL          -9
#define SPK_ERR_ALREADY_INIT      -10

#define SPK_CAT_UNKNOWN 0
#define SPK_CAT_FOREX   1
#define SPK_CAT_METAL   2
#define SPK_CAT_INDEX   3
#define SPK_CAT_CRYPTO  4
#define SPK_CAT_ENERGY  5

#define SPK_BUY   0
#define SPK_SELL  1

#define SPK_PAT_NONE             0
#define SPK_PAT_FALSE_BREAKOUT   1
#define SPK_PAT_CONSOLIDATION    2
#define SPK_PAT_IMPULSE_BREAKOUT 3

#define SPK_LOG_ERROR 0
#define SPK_LOG_WARN  1
#define SPK_LOG_INFO  2
#define SPK_LOG_DEBUG 3

#define SPK_MAX_SYMBOLS     64
#define SPK_MAX_SIGNALS     128

struct SpkBar {
    datetime time;
    double   open;
    double   high;
    double   low;
    double   close;
    long     volume;
    int      spread;
};

struct SpkSymbol {
    string   name;
    string   base;
    string   quote;
    int      category;
    int      digits;
    double   point;
    double   tick_value;
    double   tick_size;
    double   contract_size;
    double   min_lot;
    double   max_lot;
    double   lot_step;
    double   stop_level_pts;
    int      spread_typical;
    int      trade_mode;
    double   swap_long;
    double   swap_short;
    double   commission_per_lot;
    double   leverage;
};

struct SpkAccount {
    double balance;
    double equity;
    double free_margin;
    double margin_used;
    double leverage;
    double min_margin_level_pct;
    double target_margin_level_pct;
    double margin_level_pct;
};

struct SpkSignal {
    string   symbol;
    datetime bar_time;
    int      side;
    int      pattern;
    double   trigger;
    double   level;
    double   stop;
    double   confidence;
    double   rr;
};

struct SpkOrder {
    string   symbol;
    int      side;
    int      magic;
    double   volume;
    double   entry_price;
    double   stop_loss;
    double   take_profit_1;
    double   take_profit_2;
    double   risk_usd;
    string   comment;
};

struct SpkPosition {
    string   symbol;
    int      id;
    int      ticket;
    int      side;
    int      entry_spread;
    bool     tp1_hit;
    bool     tp2_hit;
    bool     be_moved;
    bool     closed;
    double   entry_price;
    double   volume_open;
    double   volume_remain;
    double   stop_loss;
    double   take_profit_1;
    double   take_profit_2;
    double   realized_pnl;
    double   total_swap;
    datetime opened_at;
    int      last_swap_day;
};

struct SpkConfig {
    double   balance;
    double   risk_percent;
    bool     compound;
    double   min_margin;
    double   min_rr;
    int      max_positions;

    bool     skip_false_breakout;
    bool     skip_impulse_buy;
    bool     skip_impulse_sell;
    bool     skip_consolidation_buy;
    bool     skip_consolidation_sell;

    bool     fx_major;
    bool     fx_cross;
    bool     metals_on;
    bool     crypto_on;
    bool     jpy_pairs;

    string   timeframe;
    int      history_bars;

    bool     session_enabled;
    int      session_start;
    int      session_end;
    int      skip_friday_after_hour;
    int      skip_monday_before_hour;

    int      max_spread_fx;
    int      max_spread_metal;
    int      max_spread_crypto;
    int      slippage_points;
    double   spread_mult;
    double   commission_mult;

    bool     trailing_enabled;
    int      trailing_distance_points;
    int      trailing_activation_points;
    bool     trail_only_after_tp1;
    bool     be_enabled;
    double   tp1_close_fraction;
    bool     tp2_full_close;
    bool     swap_enabled;

    bool     emergency_enabled;
    double   min_margin_level_pct;
    double   emergency_drawdown_pct;

    bool     log_enabled;
    string   log_dir;
    int      log_level;
    bool     trades_csv;

    int      magic;
    bool     allow_weekend;
};

#endif