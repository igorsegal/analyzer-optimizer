# ============================================================================
# ============================================================================
# GRAAL TRADING SYSTEM - C++ BUILD CONFIGURATION
# ============================================================================
# Минимальная версия CMake
cmake_minimum_required(VERSION 3.15)

# Название проекта и версия
project(GRAAL VERSION 1.0.0 LANGUAGES CXX)

# Стандарт C++ (требуется C++17 для std::optional)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Оптимизация для Release сборки
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# ============================================================================
# ИСХОДНЫЕ ФАЙЛЫ
# ============================================================================

# Основные исходники (библиотека)
set(SOURCES
    src/structures.h
    src/data_loader.h
    src/data_loader.cpp
    src/block_04_1_consolidation_detector.h
    src/block_04_1_consolidation_detector.cpp
)

# Тестовые файлы
set(TEST_SOURCES
    src/structures.h
    src/data_loader.h
    src/data_loader.cpp
    src/block_04_1_consolidation_detector.h
    src/block_04_1_consolidation_detector.cpp
)

# ============================================================================
# ОСНОВНАЯ БИБЛИОТЕКА (GRAAL Core)
# ============================================================================
add_library(graal_core STATIC ${SOURCES})

# Пули для библиотеки
target_include_directories(graal_core PUBLIC ${CMAKE_SOURCE_DIR}/src)

# ============================================================================
# ПАПКА ВЫХОДА (build/)
# ============================================================================
# Устанавливаем директорию для исполняемых файлов
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/build)

# ============================================================================
# КОПИРОВАНИЕ ДАННЫХ (опционально)
# ============================================================================
# Копируем config_01.yaml в папку сборки для удобства
file(COPY ${CMAKE_SOURCE_DIR}/config_01.yaml DESTINATION ${CMAKE_SOURCE_DIR}/build)

# ============================================================================
# СООБЩЕНИЕ ПРИ СБОРКЕ
# ============================================================================
message(STATUS "========================================")
message(STATUS "GRAAL C++ Build Configuration")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output Directory: ${CMAKE_SOURCE_DIR}/build")
message(STATUS "========================================")
