# =============================================================================
#  SPARTAK :: installer
#  Разворачивает дерево проекта и заполняет core/ боевым содержимым,
#  остальные модули — заглушками-скелетами для последующего наполнения.
#
#  Запуск:
#    powershell -ExecutionPolicy Bypass -File install_spartak.ps1
#    либо двойной клик по install.bat
# =============================================================================

[CmdletBinding()]
param(
    [string]$Root = ""
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if (-not $Root) {
    $Root = Split-Path -Parent $MyInvocation.MyCommand.Path
    if (-not $Root) { $Root = (Get-Location).Path }
}
if (-not (Test-Path -LiteralPath $Root)) {
    throw "Root folder does not exist: $Root"
}

Write-Host ""
Write-Host "=== SPARTAK :: installer ===" -ForegroundColor Cyan
Write-Host "Root: $Root" -ForegroundColor Cyan
Write-Host ""

# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------
function New-Dir([string]$rel) {
    $p = Join-Path $Root $rel
    if (-not (Test-Path -LiteralPath $p)) {
        New-Item -ItemType Directory -Force -Path $p | Out-Null
    }
    Write-Host "  dir  $rel" -ForegroundColor DarkGray
}

function Set-File([string]$rel, [string]$content) {
    $p   = Join-Path $Root $rel
    $dir = Split-Path -Parent $p
    if ($dir -and -not (Test-Path -LiteralPath $dir)) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }
    $utf8 = New-Object System.Text.UTF8Encoding($true)
    [System.IO.File]::WriteAllText($p, $content, $utf8)
    Write-Host "  file $rel" -ForegroundColor Green
}

function Set-Stub([string]$rel, [string]$ns, [string]$comment = "") {
    if (Test-Path -LiteralPath (Join-Path $Root $rel)) {
        Write-Host "  skip $rel (already exists)" -ForegroundColor Yellow
        return
    }
    $header = "#pragma once`r`n`r`n// SPARTAK :: $rel`r`n"
    if ($comment) { $header += "// $comment`r`n" }
    $header += "// TODO: implementation pending.`r`n`r`nnamespace $ns {`r`n`r`n} // namespace $ns`r`n"
    Set-File $rel $header
}

# -----------------------------------------------------------------------------
# 1. Directory tree
# -----------------------------------------------------------------------------
Write-Host "[1/3] Creating directory tree..." -ForegroundColor Yellow

$dirs = @(
    "include/core",
    "include/data",
    "include/context",
    "include/patterns",
    "include/validation",
    "include/position",
    "include/engine",
    "src/core",
    "src/data",
    "src/context",
    "src/patterns",
    "src/validation",
    "src/position",
    "src/engine",
    "tests/core",
    "tests/context",
    "tests/patterns",
    "tests/validation",
    "tests/position",
    "tests/engine",
    "config",
    "data"
)
foreach ($d in $dirs) { New-Dir $d }

# -----------------------------------------------------------------------------
# 2. Core — боевые файлы
# -----------------------------------------------------------------------------
Write-Host ""
Write-Host "[2/3] Writing core/ (production content)..." -ForegroundColor Yellow

# ---- Types.h ---------------------------------------------------------------
Set-File "include/core/Types.h" @'
// =============================================================================
//  SPARTAK :: core/Types.h
//  Базовые типы данных для всего конвейера стратегии «Зри в Корень» (ТАП).
//
//  Файл содержит ТОЛЬКО структуры и enum'ы, без логики.
//  Все модули бэктестера обмениваются через эти типы снизу вверх:
//
//     Bar / Tick  ->  MarketContext  ->  PatternSignal  ->  ValidatedOrderRequest
// =============================================================================

#pragma once

#include <cstdint>
#include <vector>

namespace spartak::core {

// -----------------------------------------------------------------------------
// 1. Рыночные данные
// -----------------------------------------------------------------------------

struct Bar {
    int64_t timestamp   = 0;   // Unix ms (открытие свечи)
    double  open        = 0.0;
    double  high        = 0.0;
    double  low         = 0.0;
    double  close       = 0.0;
    int64_t tick_volume = 0;
    int32_t spread      = 0;   // спред в пунктах
};

struct Tick {
    int64_t timestamp     = 0;
    double  bid           = 0.0;
    double  ask           = 0.0;
    int32_t spread_points = 0;
};

// -----------------------------------------------------------------------------
// 2. Тренд и уровни
// -----------------------------------------------------------------------------

enum class TrendDirection {
    Undefined,
    Bullish,   // БТ
    Bearish    // МТ
};

enum class LevelType {
    LocalLevel,          // ЛУ
    IntermediateLevel    // ПУ
};

struct PriceZone {
    LevelType type        = LevelType::LocalLevel;
    double    price_level = 0.0;
    double    zone_top    = 0.0;
    double    zone_bottom = 0.0;
    bool      is_active   = true;
};

// -----------------------------------------------------------------------------
// 3. Контекст рынка (выход ContextAggregator)
// -----------------------------------------------------------------------------

struct MarketContext {
    TrendDirection         daily_trend  = TrendDirection::Undefined;
    TrendDirection         hourly_trend = TrendDirection::Undefined;
    std::vector<PriceZone> active_zones;
    bool                   has_hh_hl_structure = false;
};

// -----------------------------------------------------------------------------
// 4. Паттерн (выход PatternAggregator)
// -----------------------------------------------------------------------------

enum class PatternType {
    None,
    FalseBreakout,
    Consolidation,
    ImpulseBreakout
};

struct PatternSignal {
    bool        detected       = false;
    PatternType type           = PatternType::None;
    double      trigger_price  = 0.0;
    double      suggested_stop = 0.0;
    double      confidence     = 0.0;
};

// -----------------------------------------------------------------------------
// 5. Одобренная заявка (выход SignalValidator)
// -----------------------------------------------------------------------------

enum class OrderSide {
    Buy,
    Sell
};

struct ValidatedOrderRequest {
    bool      is_approved   = false;
    OrderSide side          = OrderSide::Buy;
    double    volume        = 0.0;
    double    entry_price   = 0.0;
    double    stop_loss     = 0.0;
    double    take_profit_1 = 0.0;
    double    take_profit_2 = 0.0;
};

} // namespace spartak::core
'@

# ---- Constants.h -----------------------------------------------------------
Set-File "include/core/Constants.h" @'
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
'@

# ---- Config.h --------------------------------------------------------------
Set-File "include/core/Config.h" @'
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
    bool        log_every_n_bars    = 20;
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
'@

# ---- Config.cpp ------------------------------------------------------------
Set-File "src/core/Config.cpp" @'
#include "core/Config.h"

namespace spartak::core {

AppConfig make_default_config() {
    AppConfig cfg;

    cfg.account.equity      = cfg.account.balance;
    cfg.account.free_margin = cfg.account.balance;
    cfg.account.margin_used = 0.0;

    cfg.position.point              = cfg.validation.point;
    cfg.position.contract_size      = cfg.validation.contract_size;
    cfg.position.min_lot            = cfg.validation.min_lot;
    cfg.position.lot_step           = cfg.validation.lot_step;
    cfg.position.commission_per_lot = cfg.account.commission_per_lot;

    cfg.feed.use_synthetic = false;

    cfg.engine.max_bars = cfg.feed.max_bars > 0
                        ? cfg.feed.max_bars
                        : defaults::MAX_BARS;

    return cfg;
}

const char* to_string(RejectReason r) noexcept {
    switch (r) {
        case RejectReason::None:            return "None";
        case RejectReason::InvalidSignal:   return "InvalidSignal";
        case RejectReason::SpreadTooHigh:   return "SpreadTooHigh";
        case RejectReason::TrendConflict:   return "TrendConflict";
        case RejectReason::InvalidStop:     return "InvalidStop";
        case RejectReason::ZeroDistance:    return "ZeroDistance";
        case RejectReason::BalanceTooLow:   return "BalanceTooLow";
        case RejectReason::BelowMinLot:     return "BelowMinLot";
        case RejectReason::MarginTooLow:    return "MarginTooLow";
        case RejectReason::MarginCall:      return "MarginCall";
        case RejectReason::VolumeClamped:   return "VolumeClamped";
        case RejectReason::NoContext:       return "NoContext";
        case RejectReason::SessionClosed:   return "SessionClosed";
        case RejectReason::SlippageTooHigh: return "SlippageTooHigh";
    }
    return "Unknown";
}

} // namespace spartak::core
'@

# -----------------------------------------------------------------------------
# 3. Заглушки для остальных модулей
# -----------------------------------------------------------------------------
Write-Host ""
Write-Host "[3/3] Writing module stubs..." -ForegroundColor Yellow

# data/
Set-Stub "include/data/XFBarReader.h" "spartak::data" "Читатель бинарного формата XFBAR001"
Set-Stub "include/data/BarStream.h"   "spartak::data" "Поток Bar из файла/синтетики"

# context/
Set-Stub "include/context/FractalPointDetector.h"  "spartak::context" "Фрактальный поиск экстремумов"
Set-Stub "include/context/HighExtractor.h"          "spartak::context" "Извлечение локальных High"
Set-Stub "include/context/LowExtractor.h"           "spartak::context" "Извлечение локальных Low"
Set-Stub "include/context/StructureValidatorHHHL.h" "spartak::context" "Проверка HH/HL или LL/LH"
Set-Stub "include/context/ShadowNoiseFilter.h"      "spartak::context" "Фильтр теней-шумов"
Set-Stub "include/context/PUZoneCalculator.h"       "spartak::context" "Зоны ПУ (Daily)"
Set-Stub "include/context/LUZoneCalculator.h"       "spartak::context" "Зоны ЛУ (H1)"
Set-Stub "include/context/OldLevelCleaner.h"        "spartak::context" "Деактивация старых уровней"
Set-Stub "include/context/TrendBiasEvaluator.h"     "spartak::context" "Оценка тренда Daily/H1"
Set-Stub "include/context/ContextAggregator.h"      "spartak::context" "Сборка MarketContext"

# patterns/
Set-Stub "include/patterns/CandleGeometryCalculator.h"   "spartak::patterns" "Геометрия свечи"
Set-Stub "include/patterns/PinbarBuyDetector.h"          "spartak::patterns" "Pinbar BUY"
Set-Stub "include/patterns/PinbarSellDetector.h"         "spartak::patterns" "Pinbar SELL"
Set-Stub "include/patterns/TickVolumeFilter.h"           "spartak::patterns" "Фильтр тикового объёма"
Set-Stub "include/patterns/BarCloseConfirmationCounter.h" "spartak::patterns" "Подтверждение закрытий"
Set-Stub "include/patterns/ATRImpulseBreakoutDetector.h"  "spartak::patterns" "Импульсный прорыв"
Set-Stub "include/patterns/InsideBarDetector.h"          "spartak::patterns" "Inside bar"
Set-Stub "include/patterns/EngulfingDetector.h"          "spartak::patterns" "Engulfing"
Set-Stub "include/patterns/PatternAggregator.h"          "spartak::patterns" "Сборка PatternSignal"

# validation/
Set-Stub "include/validation/SpreadFilter.h"              "spartak::validation" "Фильтр спреда"
Set-Stub "include/validation/CounterTrendPUChecker.h"     "spartak::validation" "Разрешение контр-тренда на ПУ"
Set-Stub "include/validation/MoneyRiskCalculator.h"       "spartak::validation" "Расчёт риска и лота"
Set-Stub "include/validation/BrokerLotRounder.h"          "spartak::validation" "Округление лота"
Set-Stub "include/validation/MarginCallChecker.h"         "spartak::validation" "Проверка MarginLevel"
Set-Stub "include/validation/SessionTimeFilter.h"         "spartak::validation" "Фильтр торговой сессии"
Set-Stub "include/validation/SlippageToleranceChecker.h"  "spartak::validation" "Фильтр проскальзывания"
Set-Stub "include/validation/SignalValidator.h"           "spartak::validation" "Оркестратор фильтров"

# position/
Set-Stub "include/position/UO1Trigger.h"                    "spartak::position" "Триггер первой цели"
Set-Stub "include/position/VolumeSplitter50.h"              "spartak::position" "Деление 50/50"
Set-Stub "include/position/BreakEvenTransfer.h"             "spartak::position" "Перенос стопа в БУ"
Set-Stub "include/position/SwapAccrualTracker.h"            "spartak::position" "Учёт свопов"
Set-Stub "include/position/CloseCommissionCalculator.h"     "spartak::position" "Комиссия при закрытии"
Set-Stub "include/position/TrailingStopManager.h"           "spartak::position" "Trailing stop"
Set-Stub "include/position/PositionStateSynchronizer.h"     "spartak::position" "Синхронизация состояния"
Set-Stub "include/position/EmergencyCloseHandler.h"         "spartak::position" "Аварийное закрытие"
Set-Stub "include/position/PositionManager.h"               "spartak::position" "Оркестратор позиций"

# engine/
Set-Stub "include/engine/BacktestPlayer.h" "spartak::engine" "Главный цикл бэктеста"

# -----------------------------------------------------------------------------
# CMakeLists.txt
# -----------------------------------------------------------------------------
Set-File "CMakeLists.txt" @'
cmake_minimum_required(VERSION 3.20)
project(SPARTAK LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

add_library(spartak_core STATIC
    src/core/Config.cpp
)

target_include_directories(spartak_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(spartak_core PUBLIC cxx_std_20)

# main.cpp появится на этапе engine/
# add_executable(spartak main.cpp)
# target_link_libraries(spartak PRIVATE spartak_core)

enable_testing()
# add_subdirectory(tests)
'@

# -----------------------------------------------------------------------------
# README.md
# -----------------------------------------------------------------------------
Set-File "README.md" @'
# SPARTAK

C++20 бэктестер стратегии «Зри в Корень» (ТАП).

## Структура

- `include/core/`       — типы, константы, конфиги (готово)
- `include/data/`       — XFBarReader, BarStream
- `include/context/`    — 9 кластеров + ContextAggregator
- `include/patterns/`   — 8 кластеров + PatternAggregator
- `include/validation/` — 7 кластеров + SignalValidator
- `include/position/`   — 8 кластеров + PositionManager
- `include/engine/`     — BacktestPlayer
- `src/`                — реализации
- `tests/`              — юнит-тесты (CMake-таргет на каждый)

## Сборка

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j
'@

# -----------------------------------------------------------------------------
# .gitignore
# -----------------------------------------------------------------------------
Set-File ".gitignore" @'
# Build
build/
out/
bin/
obj/
*.dir/

# CMake
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
CTestTestfile.cmake
install_manifest.txt
Makefile
build.ninja
.ninja_deps
.ninja_log

# Visual Studio
.vs/
*.user
*.suo
*.sdf
*.opensdf
*.vcxproj
*.vcxproj.filters
*.sln
*.slnx
x64/
Release/
Debug/
RelWithDebInfo/
MinSizeRel/
ipch/

# MSVC intermediate
*.obj
*.iobj
*.pdb
*.ipdb
*.ilk
*.exp
*.lib
*.tlog
*.log
*.idb
*.pch
*.res
*.recipe

# System
Thumbs.db
desktop.ini
.DS_Store
'@

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------
Write-Host ""
Write-Host "=== DONE ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Layout created under: $Root"
Write-Host ""
Write-Host "Fully implemented:" -ForegroundColor Green
Write-Host "  include/core/Types.h"
Write-Host "  include/core/Constants.h"
Write-Host "  include/core/Config.h"
Write-Host "  src/core/Config.cpp"
Write-Host "  CMakeLists.txt"
Write-Host "  README.md"
Write-Host "  .gitignore"
Write-Host ""
Write-Host "Stubs (to be filled in later steps):" -ForegroundColor Yellow
Write-Host "  include/data/*        — XFBarReader, BarStream"
Write-Host "  include/context/*     — 10 files"
Write-Host "  include/patterns/*    — 9 files"
Write-Host "  include/validation/*  — 8 files"
Write-Host "  include/position/*    — 9 files"
Write-Host "  include/engine/*      — BacktestPlayer"
Write-Host ""