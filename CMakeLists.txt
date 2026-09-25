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
)
target_include_directories(spartak_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(spartak_core PUBLIC cxx_std_20)
# -----------------------------------------------------------------------------
#  Smoke tests (temporary)
# -----------------------------------------------------------------------------
add_executable(smoke_data smoke_data.cpp)
target_link_libraries(smoke_data PRIVATE spartak_core)
add_executable(smoke_file smoke_file.cpp)
target_link_libraries(smoke_file PRIVATE spartak_core)
add_executable(smoke_sanitize smoke_sanitize.cpp)
target_link_libraries(smoke_sanitize PRIVATE spartak_core)
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