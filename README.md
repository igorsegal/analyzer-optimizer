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