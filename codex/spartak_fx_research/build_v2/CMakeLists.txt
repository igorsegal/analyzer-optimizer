cmake_minimum_required(VERSION 3.20)
project(spartak_fx_research_v2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(spartak_core
    ../src/strategy.cpp
    ../src/backtest.cpp
    ../src/split.cpp
    ../src/optimizer.cpp
    ../src/data_v2.cpp
)
target_include_directories(spartak_core PUBLIC ../include)

if(MSVC)
    target_compile_options(spartak_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(spartak_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

add_executable(spartak_backtest_tests ../tests/test_backtest_fixed.cpp)
target_link_libraries(spartak_backtest_tests PRIVATE spartak_core)

add_executable(spartak_data_tests ../tests/test_data_v2.cpp)
target_link_libraries(spartak_data_tests PRIVATE spartak_core)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME backtest_engine COMMAND spartak_backtest_tests)
    add_test(NAME data_layer_v2 COMMAND spartak_data_tests)
endif()
