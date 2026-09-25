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
# -----------------------------------------------------------------------------
#  Core library
# -----------------------------------------------------------------------------
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
)
target_include_directories(spartak_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(spartak_core PUBLIC cxx_std_20)
# -----------------------------------------------------------------------------
#  Tools
# -----------------------------------------------------------------------------
add_executable(csv2xfbar tools/csv2xfbar.cpp)
# -----------------------------------------------------------------------------
#  Smoke tests (temporary)
# -----------------------------------------------------------------------------
add_executable(smoke_data       smoke_data.cpp)
add_executable(smoke_file       smoke_file.cpp)
add_executable(smoke_sanitize   smoke_sanitize.cpp)
add_executable(smoke_fractal    smoke_fractal.cpp)
add_executable(smoke_extractors smoke_extractors.cpp)
add_executable(smoke_structure  smoke_structure.cpp)
add_executable(smoke_shadow     smoke_shadow.cpp)
add_executable(smoke_zones      smoke_zones.cpp)
add_executable(smoke_cleaner    smoke_cleaner.cpp)
add_executable(smoke_bias       smoke_bias.cpp)
add_executable(smoke_context    smoke_context.cpp)
target_link_libraries(smoke_data       PRIVATE spartak_core)
target_link_libraries(smoke_file       PRIVATE spartak_core)
target_link_libraries(smoke_sanitize   PRIVATE spartak_core)
target_link_libraries(smoke_fractal    PRIVATE spartak_core)
target_link_libraries(smoke_extractors PRIVATE spartak_core)
target_link_libraries(smoke_structure  PRIVATE spartak_core)
target_link_libraries(smoke_shadow     PRIVATE spartak_core)
target_link_libraries(smoke_zones      PRIVATE spartak_core)
target_link_libraries(smoke_cleaner    PRIVATE spartak_core)
target_link_libraries(smoke_bias       PRIVATE spartak_core)
target_link_libraries(smoke_context    PRIVATE spartak_core)
# -----------------------------------------------------------------------------
#  Main executable (активируется на этапе engine/)
# -----------------------------------------------------------------------------
# add_executable(spartak main.cpp)
# target_link_libraries(spartak PRIVATE spartak_core)
# -----------------------------------------------------------------------------
#  Tests
# -----------------------------------------------------------------------------
enable_testing()
# add_subdirectory(tests)