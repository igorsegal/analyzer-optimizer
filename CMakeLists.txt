cmake_minimum_required(VERSION 3.20)
project(SPARTAK LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /wd4996 /wd4834)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

add_library(spartak_core STATIC
    src/core/Config.cpp
    src/core/InstrumentRegistry.cpp
    src/data/XFBarReader.cpp
    src/data/BarStream.cpp
    src/data/DataSanitizer.cpp
    src/context/FractalPointDetector.cpp
    src/context/HighExtractor.cpp
    src/context/LowExtractor.cpp
    src/context/StructureValidatorHHHL.cpp
    src/context/ShadowNoiseFilter.cpp
    src/context/PUZoneCalculator.cpp
    src/context/LUZoneCalculator.cpp
    src/context/OldLevelCleaner.cpp
    src/context/TrendBiasEvaluator.cpp
    src/context/ContextAggregator.cpp
    src/context/SourceZoneDetector.cpp
    src/context/BalanceShiftDetector.cpp
    src/patterns/CandleGeometryCalculator.cpp
    src/patterns/TickVolumeFilter.cpp
    src/patterns/BarCloseConfirmationCounter.cpp
    src/patterns/PinbarBuyDetector.cpp
    src/patterns/PinbarSellDetector.cpp
    src/patterns/InsideBarDetector.cpp
    src/patterns/EngulfingDetector.cpp
    src/patterns/ATRImpulseBreakoutDetector.cpp
    src/patterns/PatternAggregator.cpp
    src/validation/SpreadFilter.cpp
    src/validation/CounterTrendPUChecker.cpp
    src/validation/MoneyRiskCalculator.cpp
    src/validation/BrokerLotRounder.cpp
    src/validation/MarginCallChecker.cpp
    src/validation/SessionTimeFilter.cpp
    src/validation/SlippageToleranceChecker.cpp
    src/validation/SignalValidator.cpp
    src/position/UO1Trigger.cpp
    src/position/VolumeSplitter50.cpp
    src/position/BreakEvenTransfer.cpp
    src/position/SwapAccrualTracker.cpp
    src/position/CloseCommissionCalculator.cpp
    src/position/TrailingStopManager.cpp
    src/position/PositionStateSynchronizer.cpp
    src/position/EmergencyCloseHandler.cpp
    src/position/PositionManager.cpp
    src/engine/QuoteRateProvider.cpp
    src/engine/PnlCalculator.cpp
    src/engine/BacktestPlayer.cpp
    src/engine/PortfolioBacktest.cpp
)
target_include_directories(spartak_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(spartak_core PUBLIC cxx_std_20)

# -----------------------------------------------------------------------------
#  Main executable
# -----------------------------------------------------------------------------
add_executable(spartak cpp/main.cpp)
target_link_libraries(spartak PRIVATE spartak_core)

# -----------------------------------------------------------------------------
#  Tools
# -----------------------------------------------------------------------------
add_executable(csv2xfbar tools/csv2xfbar.cpp)

add_executable(portfolio_runner tools/portfolio_runner.cpp)
target_link_libraries(portfolio_runner PRIVATE spartak_core)

# -----------------------------------------------------------------------------
#  Smoke tests
# -----------------------------------------------------------------------------
add_executable(smoke_data         cpp/smoke_data.cpp)
add_executable(smoke_file         cpp/smoke_file.cpp)
add_executable(smoke_sanitize     cpp/smoke_sanitize.cpp)
add_executable(smoke_fractal      cpp/smoke_fractal.cpp)
add_executable(smoke_extractors   cpp/smoke_extractors.cpp)
add_executable(smoke_structure    cpp/smoke_structure.cpp)
add_executable(smoke_shadow       cpp/smoke_shadow.cpp)
add_executable(smoke_zones        cpp/smoke_zones.cpp)
add_executable(smoke_cleaner      cpp/smoke_cleaner.cpp)
add_executable(smoke_bias         cpp/smoke_bias.cpp)
add_executable(smoke_context      cpp/smoke_context.cpp)
add_executable(smoke_candle       cpp/smoke_candle.cpp)
add_executable(smoke_volume       cpp/smoke_volume.cpp)
add_executable(smoke_confirmation cpp/smoke_confirmation.cpp)
add_executable(smoke_pinbar       cpp/smoke_pinbar.cpp)
add_executable(smoke_inside       cpp/smoke_inside.cpp)
add_executable(smoke_engulfing    cpp/smoke_engulfing.cpp)
add_executable(smoke_impulse      cpp/smoke_impulse.cpp)
add_executable(smoke_pattern      cpp/smoke_pattern.cpp)
add_executable(smoke_spread       cpp/smoke_spread.cpp)
add_executable(smoke_counter      cpp/smoke_counter.cpp)
add_executable(smoke_money        cpp/smoke_money.cpp)
add_executable(smoke_lot          cpp/smoke_lot.cpp)
add_executable(smoke_margin       cpp/smoke_margin.cpp)
add_executable(smoke_session      cpp/smoke_session.cpp)
add_executable(smoke_slippage     cpp/smoke_slippage.cpp)
add_executable(smoke_validator    cpp/smoke_validator.cpp)
add_executable(smoke_trigger      cpp/smoke_trigger.cpp)
add_executable(smoke_breakeven    cpp/smoke_breakeven.cpp)
add_executable(smoke_swap         cpp/smoke_swap.cpp)
add_executable(smoke_commission   cpp/smoke_commission.cpp)
add_executable(smoke_trailing     cpp/smoke_trailing.cpp)
add_executable(smoke_emergency    cpp/smoke_emergency.cpp)

target_link_libraries(smoke_data         PRIVATE spartak_core)
target_link_libraries(smoke_file         PRIVATE spartak_core)
target_link_libraries(smoke_sanitize     PRIVATE spartak_core)
target_link_libraries(smoke_fractal      PRIVATE spartak_core)
target_link_libraries(smoke_extractors   PRIVATE spartak_core)
target_link_libraries(smoke_structure    PRIVATE spartak_core)
target_link_libraries(smoke_shadow       PRIVATE spartak_core)
target_link_libraries(smoke_zones        PRIVATE spartak_core)
target_link_libraries(smoke_cleaner      PRIVATE spartak_core)
target_link_libraries(smoke_bias         PRIVATE spartak_core)
target_link_libraries(smoke_context      PRIVATE spartak_core)
target_link_libraries(smoke_candle       PRIVATE spartak_core)
target_link_libraries(smoke_volume       PRIVATE spartak_core)
target_link_libraries(smoke_confirmation PRIVATE spartak_core)
target_link_libraries(smoke_pinbar       PRIVATE spartak_core)
target_link_libraries(smoke_inside       PRIVATE spartak_core)
target_link_libraries(smoke_engulfing    PRIVATE spartak_core)
target_link_libraries(smoke_impulse      PRIVATE spartak_core)
target_link_libraries(smoke_pattern      PRIVATE spartak_core)
target_link_libraries(smoke_spread       PRIVATE spartak_core)
target_link_libraries(smoke_counter      PRIVATE spartak_core)
target_link_libraries(smoke_money        PRIVATE spartak_core)
target_link_libraries(smoke_lot          PRIVATE spartak_core)
target_link_libraries(smoke_margin       PRIVATE spartak_core)
target_link_libraries(smoke_session      PRIVATE spartak_core)
target_link_libraries(smoke_slippage     PRIVATE spartak_core)
target_link_libraries(smoke_validator    PRIVATE spartak_core)
target_link_libraries(smoke_trigger      PRIVATE spartak_core)
target_link_libraries(smoke_breakeven    PRIVATE spartak_core)
target_link_libraries(smoke_swap         PRIVATE spartak_core)
target_link_libraries(smoke_commission   PRIVATE spartak_core)
target_link_libraries(smoke_trailing     PRIVATE spartak_core)
target_link_libraries(smoke_emergency    PRIVATE spartak_core)

enable_testing()
