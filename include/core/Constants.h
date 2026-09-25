// =============================================================================
//  SPARTAK :: core/Constants.h
//  Константы, коды ошибок и magic values.
// =============================================================================

#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

namespace spartak::core {

// -----------------------------------------------------------------------------
// 1. Формат XFBAR
// -----------------------------------------------------------------------------
namespace xfbar {
    inline constexpr std::string_view MAGIC = "XFBAR001";
    inline constexpr std::size_t      MAGIC_SIZE = 8;
    inline constexpr int32_t          VERSION = 1;
    inline constexpr std::size_t      HEADER_FIXED_SIZE = 60;
    inline constexpr std::size_t      RECORD_SIZE = 60;
    inline constexpr int32_t          MAX_SYMBOL_LEN = 255;
    inline constexpr std::size_t      DEFAULT_CHUNK_BARS = 8192;
}

// -----------------------------------------------------------------------------
// 2. Причины отказа
// -----------------------------------------------------------------------------
enum class RejectReason : int {
    None            = 0,
    InvalidSignal   = 1,
    SpreadTooHigh   = 2,
    TrendConflict   = 3,
    InvalidStop     = 4,
    ZeroDistance    = 5,
    BalanceTooLow   = 6,
    BelowMinLot     = 7,
    MarginTooLow    = 8,
    MarginCall      = 9,
    VolumeClamped   = 10,
    NoContext       = 11,
    SessionClosed   = 12,
    SlippageTooHigh = 13,
};

[[nodiscard]] const char* to_string(RejectReason r) noexcept;

// -----------------------------------------------------------------------------
// 3. Дефолтные значения (институциональные)
// -----------------------------------------------------------------------------
namespace defaults {
    inline constexpr double BALANCE              = 10'000.0;
    inline constexpr double LEVERAGE             = 500.0;
    inline constexpr double COMMISSION_PER_LOT   = 5.0;
    inline constexpr double MIN_MARGIN_LEVEL_PCT = 5000.0;

    inline constexpr double CONTRACT_SIZE        = 100'000.0;
    inline constexpr double POINT                = 0.00001;
    inline constexpr int    DIGITS               = 5;

    inline constexpr double MIN_LOT              = 0.01;
    inline constexpr double MAX_LOT              = 50.0;
    inline constexpr double LOT_STEP             = 0.01;

    inline constexpr double RISK_PERCENT         = 1.0;
    inline constexpr double TP_RISK_RATIO        = 2.0;

    inline constexpr double SWAP_LONG_POINTS     = -7.0;
    inline constexpr double SWAP_SHORT_POINTS    = -2.0;

    inline constexpr int32_t MAX_SPREAD_POINTS   = 25;
    inline constexpr int32_t FALLBACK_SPREAD     = 12;

    inline constexpr int         EXTREMUM_RADIUS   = 3;
    inline constexpr int         OFFSET_POINTS     = 15;
    inline constexpr std::size_t TREND_LOOKBACK    = 3;
    inline constexpr std::size_t MAX_ZONES_PER_TF  = 5;

    inline constexpr double WICK_RATIO_MIN      = 0.25;
    inline constexpr double BODY_RATIO_MIN      = 0.10;
    inline constexpr int    CONSOLIDATION_BARS  = 3;
    inline constexpr double IMPULSE_BODY_RATIO  = 0.55;
    inline constexpr double IMPULSE_CLOSE_RATIO = 0.70;
    inline constexpr int    STOP_BUFFER_POINTS  = 30;
    inline constexpr int    PATTERN_LOOKBACK    = 30;
    inline constexpr int    MAX_PIERCE_POINTS   = 150;

    inline constexpr double PARTIAL_PCT         = 0.50;
    inline constexpr int    BE_CLAMP_BUFFER     = 1;

    inline constexpr std::size_t MAX_BARS       = 100'000;
}

// -----------------------------------------------------------------------------
// 4. Числовой эпсилон
// -----------------------------------------------------------------------------
namespace numeric {
    inline constexpr double  LOT_EPS       = 1e-9;
    inline constexpr double  PRICE_EPS     = 1e-12;
    inline constexpr double  LOT_ROUND_MUL = 1e8;
    inline constexpr int64_t MS_PER_DAY    = 86'400'000LL;
    inline constexpr int64_t MS_PER_HOUR   = 3'600'000LL;
}

} // namespace spartak::core