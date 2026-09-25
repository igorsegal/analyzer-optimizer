// =============================================================================
//  SPARTAK :: core/Config.h
//  Единый конфиг приложения. Все модули читают свои секции отсюда.
// =============================================================================

#pragma once

#include "core/Constants.h"
#include <cstdint>
#include <string>

namespace spartak::core {

struct AccountConfig {
    double balance              = defaults::BALANCE;
    double equity               = defaults::BALANCE;
    double free_margin          = defaults::BALANCE;
    double margin_used          = 0.0;
    double leverage             = defaults::LEVERAGE;
    double commission_per_lot   = defaults::COMMISSION_PER_LOT;
    double min_margin_level_pct = defaults::MIN_MARGIN_LEVEL_PCT;
};

struct FeedConfig {
    std::string file_path;
    bool        use_synthetic  = false;
    std::size_t synthetic_bars = 1000;
    std::size_t chunk_bars     = defaults::MAX_BARS;
    std::size_t max_bars       = 0;
};

struct ContextConfig {
    double      point              = defaults::POINT;
    int         extremum_radius    = defaults::EXTREMUM_RADIUS;
    int         offset_points      = defaults::OFFSET_POINTS;
    std::size_t trend_lookback     = defaults::TREND_LOOKBACK;
    std::size_t max_zones_per_tf   = defaults::MAX_ZONES_PER_TF;
    int         old_level_age_bars = 500;
};

struct PatternConfig {
    double point               = defaults::POINT;
    double wick_ratio_min      = defaults::WICK_RATIO_MIN;
    double body_ratio_min      = defaults::BODY_RATIO_MIN;
    int    consolidation_bars  = defaults::CONSOLIDATION_BARS;
    double impulse_body_ratio  = defaults::IMPULSE_BODY_RATIO;
    double impulse_close_ratio = defaults::IMPULSE_CLOSE_RATIO;
    int    stop_buffer_points  = defaults::STOP_BUFFER_POINTS;
    int    lookback_bars       = defaults::PATTERN_LOOKBACK;
    int    max_pierce_points   = defaults::MAX_PIERCE_POINTS;
    int    volume_min          = 0;
};

struct ValidationConfig {
    double  point                     = defaults::POINT;
    double  contract_size             = defaults::CONTRACT_SIZE;
    double  risk_percent              = defaults::RISK_PERCENT;
    double  tp_risk_ratio             = defaults::TP_RISK_RATIO;
    double  min_lot                   = defaults::MIN_LOT;
    double  max_lot                   = defaults::MAX_LOT;
    double  lot_step                  = defaults::LOT_STEP;
    int32_t max_allowed_spread        = defaults::MAX_SPREAD_POINTS;
    int     stop_buffer_points        = 0;
    bool    allow_counter_trend_on_pu = true;
    bool    use_daily_trend           = true;
    bool    use_hourly_fallback       = true;
    int     session_begin_hour_utc    = 0;
    int     session_end_hour_utc      = 24;
    int32_t max_slippage_points       = 5;
};

struct PositionConfig {
    double  point                  = defaults::POINT;
    double  contract_size          = defaults::CONTRACT_SIZE;
    double  min_lot                = defaults::MIN_LOT;
    double  lot_step               = defaults::LOT_STEP;
    double  partial_pct            = defaults::PARTIAL_PCT;
    bool    use_spread_in_be       = true;
    int32_t fallback_spread_points = defaults::FALLBACK_SPREAD;
    int     be_clamp_buffer_points = defaults::BE_CLAMP_BUFFER;
    double  commission_per_lot     = defaults::COMMISSION_PER_LOT;
    double  swap_long_points       = defaults::SWAP_LONG_POINTS;
    double  swap_short_points      = defaults::SWAP_SHORT_POINTS;
    int     rollover_hour_utc      = 21;
};

struct EngineConfig {
    std::size_t max_bars            = defaults::MAX_BARS;
    bool        verbose             = true;
    std::size_t log_every_n_bars    = 20;
    bool        stop_on_margin_call = false;
};

struct AppConfig {
    AccountConfig    account;
    FeedConfig       feed;
    ContextConfig    context;
    PatternConfig    pattern;
    ValidationConfig validation;
    PositionConfig   position;
    EngineConfig     engine;
};

[[nodiscard]] AppConfig make_default_config();

} // namespace spartak::core