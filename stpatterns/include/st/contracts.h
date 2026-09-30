// =============================================================================
//  STPatterns :: contracts.h
//  Types for ST Patterns emulator.
// =============================================================================
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace st {

struct Fractal {
    int     type;          // +1 up, -1 down
    int     bar_idx;       // index in bars vector
    double  price;         // high for up, low for down
    bool    fully_formed;  // true if right neighbor exists
};

struct Corridor {
    int     dir;           // +1 buy setup (break up), -1 sell setup (break down)
    int     broken_idx;    // bar index of broken fractal
    int     opposite_idx;  // bar index of opposite fractal (stop)
    double  start_line;    // broken fractal level
    double  stop_line;     // opposite fractal level
    double  height;        // |start - stop|
    int     height_pts;    // in points
    bool    valid;         // passes ADR filter
};

struct EntrySignal {
    int     dir;
    int     bar_idx;
    double  entry_price;   // start_line +/- threshold
    Corridor corridor;
};

struct Position {
    int      dir;            // +1 buy, -1 sell
    int      bar_open;
    int      bar_close;
    double   entry_price;
    double   sl;
    double   tp1;            // 210%
    double   tp2;            // 400%
    double   volume;
    bool     be_moved;       // SL moved to BE at 210%
    bool     closed;
    double   exit_price;
    double   pnl_usd;
    std::string exit_reason; // "sl" | "tp2" | "reverse" | "be"
};

struct Account {
    double balance;
    double equity;
    double margin_used;
    double leverage;
    double min_margin_level_pct;
};

struct Config {
    // Fractal
    int      fractal_left;        // 1
    int      fractal_right;       // 1

    // ADR
    int      adr_lookback_days;   // 5
    double   adr_max_mult;        // 0.5 - max corridor height / ADR

    // Entry
    double   threshold_pts;       // 7

    // Targets
    double   tp1_mult;            // 2.1
    double   tp2_mult;            // 4.0

    // Portfolio
    double   initial_balance;     // 10000
    double   leverage;            // 500
    double   min_margin_level_pct;// 500
    double   risk_percent;        // 0.45
    int      max_positions;       // 0 = unlimited

    // Mode
    bool     swing_mode;          // false = zero-cross trigger, true = HL
    int      reversal_zone_bars;  // ~24 for H1
};

} // namespace st