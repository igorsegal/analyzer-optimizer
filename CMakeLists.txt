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
)
target_include_directories(spartak_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(spartak_core PUBLIC cxx_std_20)
add_executable(csv2xfbar tools/csv2xfbar.cpp)
add_executable(smoke_data         smoke_data.cpp)
add_executable(smoke_file         smoke_file.cpp)
add_executable(smoke_sanitize     smoke_sanitize.cpp)
add_executable(smoke_fractal      smoke_fractal.cpp)
add_executable(smoke_extractors   smoke_extractors.cpp)
add_executable(smoke_structure    smoke_structure.cpp)
add_executable(smoke_shadow       smoke_shadow.cpp)
add_executable(smoke_zones        smoke_zones.cpp)
add_executable(smoke_cleaner      smoke_cleaner.cpp)
add_executable(smoke_bias         smoke_bias.cpp)
add_executable(smoke_context      smoke_context.cpp)
add_executable(smoke_candle       smoke_candle.cpp)
add_executable(smoke_volume       smoke_volume.cpp)
add_executable(smoke_confirmation smoke_confirmation.cpp)
add_executable(smoke_pinbar       smoke_pinbar.cpp)
add_executable(smoke_inside       smoke_inside.cpp)
add_executable(smoke_engulfing    smoke_engulfing.cpp)
add_executable(smoke_impulse      smoke_impulse.cpp)
add_executable(smoke_pattern      smoke_pattern.cpp)
add_executable(smoke_spread       smoke_spread.cpp)
add_executable(smoke_counter      smoke_counter.cpp)
add_executable(smoke_money        smoke_money.cpp)
add_executable(smoke_lot          smoke_lot.cpp)
add_executable(smoke_margin       smoke_margin.cpp)
add_executable(smoke_session      smoke_session.cpp)
add_executable(smoke_slippage     smoke_slippage.cpp)
add_executable(smoke_validator    smoke_validator.cpp)
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
enable_testing()