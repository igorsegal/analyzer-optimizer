# SPARTAK :: ARCHITECTURE
Обновлено: 2026-09-30
## 1. Паспорт
- Назначение: C++20 бэктестер стратегии «Зри в Корень» (ТАП).
- Сборка: CMake >= 3.20, MSVC 18.8, C++20.
- Состояние: spartak_core.lib + spartak.exe собираются.
## 2. Данные
- Каталог: D:\AHexaTrader\1DataFiles\raw
- 300+ символов: FX, металлы, крипта, индексы, акции, товары.
- Формат: XFBAR001 (.bin), таймфреймы D1, H4, H1, M30, M15, M5.
## 3. Слои
- core/       — типы, константы, конфиг, реестр инструментов
- data/       — XFBarReader, BarStream, DataSanitizer
- context/    — фракталы, зоны PU/LU, тренд, очистка уровней
- patterns/   — пинбары, поглощения, inside, импульс, агрегатор
- validation/ — спред, маржа, риск, лот, сессия, валидатор
- position/   — триггеры, сплит, BE, трейлинг, свопы, менеджер
- engine/     — BacktestPlayer, PortfolioBacktest, PnL, курсы
## 4. Исполняемые цели
- spartak           — одиночный бэктест по одному XFBAR
- portfolio_runner  — портфельный бэктест
- csv2xfbar         — CSV -> XFBAR001
- 33 smoke-теста
Исключено из сборки: stpatterns/, st_emulator. Перенесены в отдельный проект.
## 5. Проверочные прогоны
EURUSD_D1.bin: 0 сделок (DataSanitizer отсёк весь D1, гэпы > 4ч).
EURUSD_H1.bin: 378 ордеров, 377 закрытий, Return +49.45%, MaxDD 17.66%.
## 6. Известные дефекты
- PROGRESS.md устарел (дата 2026-09-24).
- README.md: patterns=8, реально 9.
- CMakeLists.txt: путь main.cpp исправлен на cpp/main.cpp, smoke_* на cpp/smoke_*.
- .gitignore: последняя строка склеена.
- docs/LIMB.md пуст (0 байт).
- CMakeLists.docx, build.log, build_st.log — мусор, удалить.