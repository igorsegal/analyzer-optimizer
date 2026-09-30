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
- CMakeLists.docx, build.log, build_st.log — мусор, удалить.---
## 5. Поток данных
    XFBAR .bin
      -> data::XFBarReader          (header + 60-byte records)
      -> data::BarStream            (push_back + синтетика)
      -> data::DataSanitizer        (отсечение нерегулярного префикса)
      -> context::ContextAggregator (fractal -> noise -> PU/LU -> trend)
      -> patterns::PatternAggregator (pinbar / inside / engulfing / impulse)
      -> validation::SignalValidator (spread / trend / risk / lot / margin)
      -> position::PositionManager   (SL / TP1 / TP2 / BE / trail / swap)
      -> engine::BacktestPlayer      (balance, equity, stats)
      -> BacktestReport              (stdout)
## 6. Стратегия (по docs/tz/zri_v_koren.md)
Термины: БТ, МТ, ИКТ, ККТ, ЛУ, ПУ, ОП, ОС, РМ, ТВ, Стоп, УБ, УО.
Таймфреймы:
- Daily  — тренд + ЛУ/ПУ на истории.
- 1H     — ЛУ/ПУ, ближайшие к цене.
- M5-M15 — РМ (разворотный момент) в области ЛУ/ПУ.
Правила ТВ BUY:
1. ПУ или ЛУ на Daily/1H.
2. Котировка попала в область ПУ или ЛУ.
3. РМ в BUY на M5-M15.
4. Стоп ниже области ПУ/ЛУ с учётом теней.
Правила ТВ SELL — зеркально.
Сопровождение:
- Фикс 50% на первой цели.
- УБ (перенос стопа в безубыток).
- Фикс 50% остатка на второй цели.
Виды РМ: закрепление на ПУ, закрепление выше ПУ, закрепление за ПУ, прорыв.
## 7. Задача оптимизации
Цель: подобрать порог пробоя (в пунктах) для определения РМ.
Гипотеза: на разных классах инструментов (FX major / cross / metal / crypto) оптимум разный.
Метод: сетка порогов x IS (3 мес) / OOS (1 мес) по 15 годам.
Кандидаты параметра «порог пробоя» в коде:
- include/context/PUZoneCalculator.h    — offset_points (сейчас 15).
- include/context/LUZoneCalculator.h    — offset_points (сейчас 10).
- include/patterns/ATRImpulseBreakoutDetector.h — atr_multiplier (1.5).
- include/patterns/PinbarBuyDetector.h  — min_lower_ratio (0.60).
- include/patterns/PinbarSellDetector.h — min_upper_ratio (0.60).
Решение: какой именно параметр оптимизируем — открыто.