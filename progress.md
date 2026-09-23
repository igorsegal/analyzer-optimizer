# Прогресс проекта эмулятора-оптимизатора «SPARTAK»

## Общая информация
- **Путь к проекту:** `D:\AHexaTrader\2026.07.21 SPARTAK`
- **Стандарт C++:** C++17
- **Компилятор:** Visual Studio 18 2026 (cl.exe)
- **Цель:** Создать эмулятор-оптимизатор для отбора топ-50 инструментов с разбивкой на группы (Spot, Crypto, Metals) и использованием ML для предсказания разворотных моментов.

## Структура каталогов

D:\AHexaTrader\2026.07.21 SPARTAK
├── include
│ ├── common.h
│ ├── DataLoader.h
│ ├── GroupClassifier.h
│ ├── WindowManager.h
│ ├── LevelDetector.h
│ ├── TrendAnalyzer.h
│ ├── FeatureExtractor.h
│ ├── MLPredictor.h
│ ├── SignalGenerator.h
│ └── ... (будут добавлены PositionManager, BacktestEngine, Optimizer, Evaluator, TopSelector, ReportExporter, ThreadPool)
├── src
│ ├── DataLoader.cpp
│ ├── GroupClassifier.cpp
│ ├── WindowManager.cpp
│ ├── LevelDetector.cpp
│ ├── TrendAnalyzer.cpp
│ ├── FeatureExtractor.cpp
│ ├── MLPredictor.cpp
│ ├── SignalGenerator.cpp
│ └── ... (будут добавлены PositionManager, BacktestEngine, Optimizer, Evaluator, TopSelector, ReportExporter, ThreadPool, main.cpp)
├── output
└── docs\

text

## Список созданных файлов с полным содержимым (кратко, чтобы восстановить контекст)

### include/common.h
Содержит все глобальные структуры: Bar, Level, Signal, Trade, Statistics, StrategyParams, Window, WindowResult, InstrumentInfo, InstrumentResult, SelectedInstrument. Также перечисления TimeFrame, Trend и глобальные константы (DATA_ROOT, OUTPUT_CSV, RECORD_SIZE, MAGIC).

### include/DataLoader.h + src/DataLoader.cpp
Класс DataLoader с методами scan (поиск файлов *_M5.bin) и loadBars (чтение XFBAR).

### include/GroupClassifier.h + src/GroupClassifier.cpp
Класс GroupClassifier, который загружает JSON-списки из папки `D:\AHexaTrader\DataFiles\groups` (group_forex.json, group_crypto.json, group_metals.json) и определяет группу по символу. Использует простой парсинг без внешних библиотек.

### include/WindowManager.h + src/WindowManager.cpp
Класс WindowManager с методом split, разбивающим историю M5 на окна по 4 месяца (3 месяца train, 1 месяц test).

### include/LevelDetector.h + src/LevelDetector.cpp
Класс LevelDetector с методом detect, находящим локальные экстремумы и кластеризующим их в уровни поддержки/сопротивления (по минимальному числу касаний и допуску в пипсах).

### include/TrendAnalyzer.h + src/TrendAnalyzer.cpp
Класс TrendAnalyzer с методом getTrend, вычисляющим EMA и определяющим тренд (BULL/BEAR/SIDEWAYS).

### include/FeatureExtractor.h + src/FeatureExtractor.cpp
Класс FeatureExtractor с методом extract, формирующим вектор признаков (8 признаков: расстояние до уровня, RSI, размер свечи, тени, отклонение от EMA, скорость, тренд) и методом normalize для Z-нормализации.

### include/MLPredictor.h + src/MLPredictor.cpp
Класс MLPredictor — логистическая регрессия с SGD, обучение по признакам и целевым меткам (разворот/не разворот), метод predict возвращает вероятность.

### include/SignalGenerator.h + src/SignalGenerator.cpp
Класс SignalGenerator с методом generate, который на основе текущего бара, уровней, тренда и ML-вероятности формирует сигнал BUY/SELL/NONE с вычислением стоп-лосса и тейк-профита.

## Текущий шаг (на чём остановились)
Закончили создание **SignalGenerator.cpp**. Следующий файл для создания — **PositionManager.h** и его реализация.

## Данные (внешние)
- Путь к данным: `D:\AHexaTrader\DataFiles\raw` — там лежат папки символов с файлами *_M5.bin, *_H1.bin, *_H4.bin, *_D1.bin (некоторые создаются при необходимости).
- Группы: `D:\AHexaTrader\DataFiles\groups` — JSON-списки.

## Команды для сборки
Для тестирования текущего состояния можно скомпилировать любой из созданных классов. Основной исполняемый файл пока не создан, но можно собирать отдельные тесты, например:
```cmd
cl /EHsc /std:c++17 /O2 main_test.cpp DataLoader.cpp GroupClassifier.cpp /Fe:test.exe
(если есть main_test.cpp)

Планы на завтра
Создать PositionManager.h и .cpp — управление открытой позицией.

Создать BacktestEngine.h и .cpp — основной движок бэктеста.

Создать Optimizer.h и .cpp — перебор параметров.

Создать Evaluator.h и .cpp — прогон на OOS.

Создать TopSelector.h и .cpp — отбор топ-50.

Создать ReportExporter.h и .cpp — экспорт CSV.

Создать ThreadPool.h (header-only) для пула потоков.

Создать main.cpp — главный цикл.

Примечания
Код использует только STL, без внешних библиотек.

Все классы находятся в пространстве имён AHexa.

Формат XFBAR: запись 60 байт, заголовок с магией.

В будущем нужно будет добавить параметр point из заголовка файла вместо жёстко заданного 0.0001 (пока упрощённо).
