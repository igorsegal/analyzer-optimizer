# SPARTAK
C++20 бэктестер стратегии «Зри в Корень» (ТАП).
## Назначение
Прогоняет историю XFBAR-файлов через пайплайн:
    Bar -> context -> patterns -> validation -> position -> engine
и выдаёт отчёт: сделки, PnL, просадку, статистику.
## Структура
    include/core/        типы, константы, конфиг, реестр инструментов
    include/data/        XFBarReader, BarStream, DataSanitizer
    include/context/     фракталы, зоны PU/LU, тренд, очистка уровней
    include/patterns/    пинбары, поглощения, inside, импульс, агрегатор
    include/validation/  спред, маржа, риск, лот, сессия, валидатор
    include/position/    триггеры, сплит, BE, трейлинг, свопы, менеджер
    include/engine/      BacktestPlayer, PortfolioBacktest, PnL, курсы
    src/                 реализации
    cpp/                 main.cpp, 36 smoke-тестов
    tools/               csv2xfbar, portfolio_runner
    docs/                tz_spartak.md (аудит ТЗ vs код), LIMB.md
    mt4/                 MQL4-порт (Experts, Include, Files)
## Сборка
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
## Запуск
Одиночный бэктест:
    build\Release\spartak.exe --file <path_to_xfbar> [--max-bars N] [--quiet]
Портфельный бэктест:
    build\Release\portfolio_runner.exe --config spartak.ini [--dir <raw_dir>] [--list]
## Данные
Формат XFBAR001 (.bin). Каталог по умолчанию:
    D:\AHexaTrader\1DataFiles\raw\<SYMBOL>\<SYMBOL>_<TF>.bin
Таймфреймы: D1, H4, H1, M30, M15, M5.
## Документация
- `ARCHITECTURE.md` — паспорт, реестр, состояние.
- `docs/tz_spartak.md` — сверка реализации с ТЗ.
## Статус
- spartak_core.lib собирается.
- spartak.exe, portfolio_runner.exe собираются.
- smoke-тесты собираются.
- Прогон EURUSD_H1: 378 ордеров, Return +49.45%, MaxDD 17.66%.