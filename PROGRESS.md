\# SPARTAK :: Progress



\## Статус на 2026-09-24



\### Готово

\- \[x] Layout v1.0 зафиксирован (split .h/.cpp, tests/ с CMake-таргетом)

\- \[x] core/Types.h        — Bar, Tick, PriceZone, MarketContext,

&#x20;                           PatternSignal, ValidatedOrderRequest, OrderSide

\- \[x] core/Constants.h    — XFBAR-константы, RejectReason, defaults, numeric

\- \[x] core/Config.h       — 7 секций AppConfig + make\_default\_config()

\- \[x] src/core/Config.cpp — реализация

\- \[x] CMakeLists.txt      — spartak\_core собирается без warning'ов

\- \[x] build/Release/spartak\_core.lib — собран



\### Следующий шаг

\*\*Шаг №4 — слой data/:\*\*

\- \[ ] include/data/XFBarReader.h   — чтение бинарника XFBAR001

\- \[ ] include/data/BarStream.h     — поток Bar (файл или синтетика)

\- \[ ] src/data/XFBarReader.cpp

\- \[ ] src/data/BarStream.cpp

\- \[ ] патч CMakeLists.txt (добавить новые .cpp в spartak\_core)

\- \[ ] smoke-тест на синтетике



\### Дальше по плану

\- Шаг №5: context/ (10 кластеров)

\- Шаг №6: patterns/ (9 кластеров)

\- Шаг №7: validation/ (8 кластеров)

\- Шаг №8: position/ (9 кластеров)

\- Шаг №9: engine/BacktestPlayer + main.cpp

\- Шаг №10: tests/ с CMake-таргетом на каждый модуль



\### Решения, зафиксированные в архитектуре

\- Split .h/.cpp (не header-only)

\- XFBarReader обязателен (реальные .bin)

\- Единый core/Config.h

\- tests/ отдельная папка, CMake-таргет на каждый тест

\- Namespace: spartak::core, spartak::data, spartak::context, ...

\- Плечо 1:500, комиссия $5/лот, minMarginLevel 5000%

\- Свопы в пунктах, начисление при переходе через полночь UTC

