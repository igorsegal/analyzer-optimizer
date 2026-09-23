cmake_minimum_required(VERSION 3.20)
project(spartak_fx_research_release LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(spartak_core
    ../src/strategy.cpp
    ../src/baseline_strategy.cpp
    ../src/backtest.cpp
    ../src/split.cpp
    ../src/optimizer.cpp
    ../src/data_v2.cpp
    ../src/specs.cpp
)
target_include_directories(spartak_core PUBLIC ../include)

if(MSVC)
    target_compile_options(spartak_core PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(spartak_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
endif()

add_executable(spartak_inspect ../src/inspect_v2.cpp)
target_link_libraries(spartak_inspect PRIVATE spartak_core)

add_executable(spartak_specs_inspect ../src/specs_inspect.cpp)
target_link_libraries(spartak_specs_inspect PRIVATE spartak_core)

add_executable(spartak_backtest_tests ../tests/test_backtest_fixed.cpp)
target_link_libraries(spartak_backtest_tests PRIVATE spartak_core)
add_executable(spartak_data_tests ../tests/test_data_v2.cpp)
target_link_libraries(spartak_data_tests PRIVATE spartak_core)
add_executable(spartak_baseline_tests ../tests/test_baseline_strategy.cpp)
target_link_libraries(spartak_baseline_tests PRIVATE spartak_core)
add_executable(spartak_specs_tests ../tests/test_specs.cpp)
target_link_libraries(spartak_specs_tests PRIVATE spartak_core)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME backtest_engine COMMAND spartak_backtest_tests)
    add_test(NAME data_layer_v2 COMMAND spartak_data_tests)
    add_test(NAME baseline_strategy COMMAND spartak_baseline_tests)
    add_test(NAME instrument_specs COMMAND spartak_specs_tests)
endif()
