// =============================================================================
//  SPARTAK :: Spartak/cfg_load.mqh
// =============================================================================
#ifndef SPARTAK_CFG_LOAD_MQH
#define SPARTAK_CFG_LOAD_MQH

#include <Spartak/contracts.mqh>
#include <Spartak/cfg_defaults.mqh>

string cfg_trim(string s)
{
    StringTrimLeft(s);
    StringTrimRight(s);
    return s;
}

bool cfg_to_bool(string v)
{
    string s = v;
    StringToLower(s);
    if (s == "true"  || s == "yes" || s == "1" || s == "on")  return true;
    if (s == "false" || s == "no"  || s == "0" || s == "off") return false;
    return false;
}

void cfg_apply(SpkConfig &cfg, string key, string val)
{
    if (key == "balance")                       cfg.balance = StringToDouble(val);
    else if (key == "risk_percent")             cfg.risk_percent = StringToDouble(val);
    else if (key == "compound")                 cfg.compound = cfg_to_bool(val);
    else if (key == "min_margin")               cfg.min_margin = StringToDouble(val);
    else if (key == "min_rr")                   cfg.min_rr = StringToDouble(val);
    else if (key == "max_positions")            cfg.max_positions = (int)StringToInteger(val);

    else if (key == "skip_false_breakout")      cfg.skip_false_breakout = cfg_to_bool(val);
    else if (key == "skip_impulse_buy")         cfg.skip_impulse_buy = cfg_to_bool(val);
    else if (key == "skip_impulse_sell")        cfg.skip_impulse_sell = cfg_to_bool(val);
    else if (key == "skip_consolidation_buy")   cfg.skip_consolidation_buy = cfg_to_bool(val);
    else if (key == "skip_consolidation_sell")  cfg.skip_consolidation_sell = cfg_to_bool(val);

    else if (key == "fx_major")                 cfg.fx_major = cfg_to_bool(val);
    else if (key == "fx_cross")                 cfg.fx_cross = cfg_to_bool(val);
    else if (key == "metals_on")                cfg.metals_on = cfg_to_bool(val);
    else if (key == "crypto_on")                cfg.crypto_on = cfg_to_bool(val);
    else if (key == "jpy_pairs")                cfg.jpy_pairs = cfg_to_bool(val);

    else if (key == "timeframe")                cfg.timeframe = val;
    else if (key == "history_bars")             cfg.history_bars = (int)StringToInteger(val);

    else if (key == "session_enabled")          cfg.session_enabled = cfg_to_bool(val);
    else if (key == "session_start")            cfg.session_start = (int)StringToInteger(val);
    else if (key == "session_end")              cfg.session_end = (int)StringToInteger(val);
    else if (key == "skip_friday_after_hour")   cfg.skip_friday_after_hour = (int)StringToInteger(val);
    else if (key == "skip_monday_before_hour")  cfg.skip_monday_before_hour = (int)StringToInteger(val);

    else if (key == "max_spread_fx")            cfg.max_spread_fx = (int)StringToInteger(val);
    else if (key == "max_spread_metal")         cfg.max_spread_metal = (int)StringToInteger(val);
    else if (key == "max_spread_crypto")        cfg.max_spread_crypto = (int)StringToInteger(val);
    else if (key == "slippage_points")          cfg.slippage_points = (int)StringToInteger(val);
    else if (key == "spread_mult")              cfg.spread_mult = StringToDouble(val);
    else if (key == "commission_mult")          cfg.commission_mult = StringToDouble(val);

    else if (key == "trailing_enabled")         cfg.trailing_enabled = cfg_to_bool(val);
    else if (key == "trailing_distance_points") cfg.trailing_distance_points = (int)StringToInteger(val);
    else if (key == "trailing_activation_points") cfg.trailing_activation_points = (int)StringToInteger(val);
    else if (key == "trail_only_after_tp1")     cfg.trail_only_after_tp1 = cfg_to_bool(val);
    else if (key == "be_enabled")               cfg.be_enabled = cfg_to_bool(val);
    else if (key == "tp1_close_fraction")       cfg.tp1_close_fraction = StringToDouble(val);
    else if (key == "tp2_full_close")           cfg.tp2_full_close = cfg_to_bool(val);
    else if (key == "swap_enabled")             cfg.swap_enabled = cfg_to_bool(val);

    else if (key == "emergency_enabled")        cfg.emergency_enabled = cfg_to_bool(val);
    else if (key == "min_margin_level_pct")     cfg.min_margin_level_pct = StringToDouble(val);
    else if (key == "emergency_drawdown_pct")   cfg.emergency_drawdown_pct = StringToDouble(val);

    else if (key == "log_enabled")              cfg.log_enabled = cfg_to_bool(val);
    else if (key == "log_dir")                  cfg.log_dir = val;
    else if (key == "log_level")
    {
        string s = val;
        StringToLower(s);
        if (s == "error")      cfg.log_level = SPK_LOG_ERROR;
        else if (s == "warn")  cfg.log_level = SPK_LOG_WARN;
        else if (s == "info")  cfg.log_level = SPK_LOG_INFO;
        else if (s == "debug") cfg.log_level = SPK_LOG_DEBUG;
    }
    else if (key == "trades_csv")               cfg.trades_csv = cfg_to_bool(val);

    else if (key == "magic")                    cfg.magic = (int)StringToInteger(val);
    else if (key == "allow_weekend")            cfg.allow_weekend = cfg_to_bool(val);
}

bool cfg_load(SpkConfig &cfg, string filename)
{
    cfg_defaults(cfg);

    int h = FileOpen(filename, FILE_READ | FILE_TXT | FILE_ANSI);
    if (h == INVALID_HANDLE)
    {
        Print("SPARTAK cfg_load: cannot open ", filename,
              ", error=", GetLastError(), ". Using defaults.");
        return false;
    }

    int line_no = 0;
    while (!FileIsEnding(h))
    {
        string line = FileReadString(h);
        line_no++;
        line = cfg_trim(line);
        if (StringLen(line) == 0) continue;
        if (StringGetChar(line, 0) == '#') continue;
        if (StringGetChar(line, 0) == ';') continue;

        int eq = StringFind(line, "=");
        if (eq <= 0) continue;

        string key = cfg_trim(StringSubstr(line, 0, eq));
        string val = cfg_trim(StringSubstr(line, eq + 1));
        if (StringLen(key) == 0) continue;

        cfg_apply(cfg, key, val);
    }
    FileClose(h);

    Print("SPARTAK cfg_load: ", filename, " loaded, ", line_no, " lines");
    return true;
}

#endif