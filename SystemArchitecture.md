# Архитектура эмулятора-оптимизатора торговой системы «Зри в корень»

## Общая схема потока данных (конвейер)
Файл SystemArchitecture.md (полная копия)
markdown
# Архитектура эмулятора-оптимизатора торговой системы «Зри в корень»

## Общая схема потока данных (конвейер)
[Файлы *.bin]
> FileScanner (список инструментов)
> DataLoader (загрузка M5 баров)
> TimeFrameAggregator (H1, H4, Daily)
> GroupClassifier (привязка группы)
> WindowManager (разбивка на окна train/test)
> (для каждого окна и инструмента)
> LevelDetector, TrendAnalyzer (вычисление уровней/тренда)
> FeatureExtractor (признаки для ML)
> MLPredictor (обучение на train, прогноз на test)
> SignalGenerator (сигналы на основе уровней + ML)
> PositionManager (управление сделками)
> BacktestEngine (сбор статистики)
> Optimizer (перебор параметров на train, выбор лучших)
> Evaluator (прогон на test с лучшими параметрами)
> (после всех окон) агрегация статистики по инструменту
> TopSelector (выбор топ-50 по группам)
> ReportExporter (CSV)

text

---

## Глобальные структуры данных

```cpp
// Базовая свеча (соответствует XFBAR)
struct Bar {
    int64_t  time;          // Unix timestamp
    double   open, high, low, close;
    int64_t  tick_volume;
    int32_t  spread;
    int64_t  real_volume;
};

// Уровень (поддержка/сопротивление)
struct Level {
    double price;
    enum Type { SUPPORT, RESISTANCE } type;
    int touches;            // количество касаний
    int64_t lastTouchTime;
};

// Сигнал на вход
struct Signal {
    enum Direction { BUY, SELL, NONE } direction;
    double entryPrice;
    double stopLoss;
    double takeProfit;
};

// Статистика по одной сделке
struct Trade {
    int64_t openTime, closeTime;
    double entryPrice, exitPrice;
    double profit;          // в валюте депозита
    double profitPips;
    bool win;
};

// Итоговая статистика за период (окно или весь инструмент)
struct Statistics {
    double totalProfit;
    double totalProfitPips;
    double maxDrawdown;
    double winRate;
    double profitFactor;
    double sharpeRatio;
    int totalTrades;
    int winTrades;
    double avgProfit;
    double avgLoss;
};

// Параметры стратегии (оптимизируемые)
struct StrategyParams {
    int    trendEmaPeriod;       // период EMA для определения тренда (20,50,100,200)
    double levelTolerancePips;   // допуск к уровню в пунктах (5-30)
    int    minTouches;           // минимальное число касаний для уровня (2-4)
    int    confirmationBars;     // число свечей для подтверждения (1-2)
    double takeProfitRatio;      // отношение TP к SL (1.5, 2, 3)
    double breakEvenPips;        // активация безубытка через N пунктов (10-50)
    double mlThreshold;          // порог вероятности ML (0.6-0.9)
    int    mlLookaheadBars;      // на сколько баров вперёд определяем разворот (3-10)
};

// Результат оптимизации для одного окна
struct WindowResult {
    StrategyParams params;
    Statistics     trainStats;  // статистика на обучающей части
    Statistics     testStats;   // статистика на тестовой части (OOS)
};

// Итоговый результат по инструменту (усреднённый по окнам)
struct InstrumentResult {
    std::string symbol;
    std::string group;          // "Spot", "Crypto", "Metals"
    std::vector<WindowResult> windowResults;
    Statistics avgTrainStats;
    Statistics avgTestStats;
    StrategyParams bestOverallParams; // параметры из окна с лучшим Sharpe на test
};
Описание блоков (входы > выходы)
1. FileScanner
Назначение	Сканирует директорию и формирует список инструментов с путями к файлам.
Вход	rootDir (std::string) – корневая папка D:\AHexaTrader\DataFiles\raw\
Выход	std::vector<InstrumentInfo> – каждый содержит: symbol, filePath, group (если может определить по имени).
Алгоритм	Обходит подпапки, ищет файлы *_M5.bin, извлекает имя символа. Группу пока не определяет – передаёт дальше.
2. DataLoader
Назначение	Читает бинарный файл XFBAR, проверяет заголовок, загружает все бары M5.
Вход	InstrumentInfo (содержит путь)
Выход	std::vector<Bar> – массив баров M5.
Алгоритм	Открывает файл, читает заголовок, проверяет магию "XFBAR001", version=1, record_size=60. Затем читает bar_count записей по 60 байт, преобразует в структуру Bar. Возвращает вектор.
3. TimeFrameAggregator
Назначение	Из баров M5 строит бары H1, H4, Daily (по правилам агрегации).
Вход	const std::vector<Bar>& barsM5
Выход	std::map<TimeFrame, std::vector<Bar>> – где TimeFrame = H1, H4, Daily.
Алгоритм	Для каждого целевого ТФ вычисляет фактор N (H1=12, H4=48, Daily=... зависит от времени). Группирует бары по времени, агрегирует: open=первый, high=max, low=min, close=последний, объёмы суммируются, спред – среднее. Отбрасывает неполные группы.
4. GroupClassifier
Назначение	Определяет группу актива (Spot, Crypto, Metals) по имени символа.
Вход	const std::string& symbol
Выход	std::string – одна из трёх групп.
Алгоритм	Использует жёсткий словарь (или конфиг): если символ содержит BTC, ETH, XRP > Crypto; если XAU, XAG, XPT > Metals; иначе > Spot. Можно также читать внешний файл mapping.
5. WindowManager
Назначение	Разбивает историю (M5 бары) на непересекающиеся окна по 4 месяца.
Вход	const std::vector<Bar>& barsM5
Выход	std::vector<Window> – каждый содержит trainBars (первые 3 мес.) и testBars (последний 1 мес.) и границы времени.
Алгоритм	Определяет первую и последнюю дату. От первой даты шагает вперёд по 4 месяца. Для каждого окна берёт 3 месяца на обучение, 1 месяц на тест. Если остаток меньше 1 месяца – отбрасывает. Возвращает вектор окон.
6. LevelDetector
Назначение	Находит локальные уровни поддержки/сопротивления на заданном ТФ (Daily или H1).
Вход	const std::vector<Bar>& bars, int lookbackBars (сколько баров анализировать)
Выход	std::vector<Level> – уровни с ценой, типом, числом касаний.
Алгоритм	Для каждого бара определяет локальный экстремум (сравнение с соседними). Кластеризует близкие цены (порог = 2-3 пункта). Ранжирует по количеству касаний, оставляет только уровни с touches >= minTouches.
7. TrendAnalyzer
Назначение	Определяет направление тренда (Bull, Bear, Sideways) на Daily/H1.
Вход	const std::vector<Bar>& bars, int emaPeriod
Выход	enum Trend { BULL, BEAR, SIDEWAYS }
Алгоритм	Вычисляет EMA за период. Сравнивает цену закрытия с EMA: если close > EMA и выше предыдущего максимума – BULL; если close < EMA – BEAR; иначе SIDEWAYS. Можно также использовать метод последовательных экстремумов.
8. FeatureExtractor
Назначение	Из последовательности баров и уровней формирует вектор числовых признаков для ML.
Вход	const std::vector<Bar>& windowBars (M5), const std::vector<Level>& levels, Trend trend
Выход	std::vector<double> – нормализованные признаки (размер фиксирован, например 20).
Алгоритм	На каждом баре (или на каждом моменте принятия решения) вычисляет:
- расстояние до ближайшего уровня (в пунктах)
- относительную силу (RSI за 14)
- размер свечи (high-low) / point
- верхняя/нижняя тень
- положение относительно EMA
- скорость изменения цены за 5 баров
- текущий тренд (закодированный)
Все признаки нормализуются (Z-score) по обучающей выборке.
9. MLPredictor
Назначение	Обучается на обучающем окне и выдаёт вероятность разворота на тестовых данных.
Вход (обучение)	std::vector<std::vector<double>> features, std::vector<int> targets (0/1)
Вход (прогноз)	std::vector<double> featureVector
Выход	double probability (0..1)
Алгоритм	Использует логистическую регрессию с L2-регуляризацией. Обучение – градиентный спуск (SGD) с фиксированным числом эпох. Целевая метка: 1, если в течение lookaheadBars цена отскочила от уровня в нужном направлении (т.е. достигнут локальный экстремум).
10. SignalGenerator
Назначение	Генерирует торговый сигнал (BUY/SELL/NONE) на основе текущего состояния.
Вход	const Bar& currentBar, const std::vector<Level>& levels, Trend trend, double mlProb, const StrategyParams& params
Выход	Signal
Алгоритм	1. Проверяет, находится ли цена в зоне уровня (close ± levelTolerancePips).
2. Проверяет, что mlProb >= mlThreshold.
3. Проверяет, что направление разворота согласовано с трендом (если тренд бычий – только BUY, медвежий – только SELL, флэт – оба).
4. Дополнительно может требовать паттерн (пин-бар, поглощение) – подтверждение свечами.
5. Вычисляет стоп-лосс (чуть ниже/выше уровня) и тейк-профит (стоп * takeProfitRatio).
11. PositionManager
Назначение	Управляет одной открытой позицией: обновляет стоп, безубыток, фиксирует частичную прибыль.
Вход	Signal (при открытии), текущий бар, StrategyParams
Выход	Trade (закрытая сделка) или nullopt (если ещё открыта)
Алгоритм	При открытии запоминает entryPrice, stopLoss, takeProfit. При каждом новом баре проверяет условия:
- если цена достигла takeProfit > закрыть с прибылью.
- если цена достигла stopLoss > закрыть с убытком.
- если цена прошла breakEvenPips в сторону прибыли – перемещает стоп на уровень безубытка.
- если цена прошла 2*breakEvenPips – фиксирует часть прибыли (передвигает стоп).
12. BacktestEngine
Назначение	Выполняет прогон стратегии на заданном наборе баров (тестовый период) и собирает статистику.
Вход	const std::vector<Bar>& testBars, const StrategyParams& params, const MLPredictor& model (уже обучена)
Выход	Statistics
Алгоритм	Инициализирует LevelDetector и TrendAnalyzer на всей истории (но для каждого бара используются только прошлые данные). Идёт по барам последовательно:
- если позиции нет – вызывает SignalGenerator (с текущим уровнем, трендом, ML-прогнозом).
- если есть – вызывает PositionManager для обновления.
- фиксирует все закрытые сделки.
- по завершении вычисляет метрики.
13. Optimizer
Назначение	Перебирает сетку параметров на обучающей выборке, выбирает комбинацию с лучшей метрикой (например, Sharpe).
Вход	const std::vector<Bar>& trainBars (3 месяца), const std::vector<Bar>& dailyBars, const std::vector<Bar>& h1Bars (для уровней и тренда)
Выход	StrategyParams – лучшие параметры.
Алгоритм	Для каждой комбинации параметров из заданных диапазонов:
- обучает ML-модель на trainBars,
- запускает BacktestEngine на той же trainBars (или на её части как валидация),
- вычисляет метрику (например, Sharpe или ProfitFactor).
- выбирает параметры с максимальной метрикой.
14. Evaluator
Назначение	Прогоняет стратегию с фиксированными параметрами на тестовом (OOS) окне.
Вход	const std::vector<Bar>& testBars, const StrategyParams& params, const MLPredictor& model (обучена на train)
Выход	Statistics – статистика на OOS.
Алгоритм	Аналогичен BacktestEngine, но использует обученную модель (без переобучения).
15. TopSelector
Назначение	Из всех инструментов выбирает топ-50 с учётом групповых лимитов.
Вход	const std::vector<InstrumentResult>& allResults, int totalCount=50, std::map<std::string, int> groupLimits (например, Spot=20, Crypto=15, Metals=15)
Выход	std::vector<SelectedInstrument> – содержит символ, группу, параметры, статистику.
Алгоритм	Группирует allResults по группам. В каждой группе сортирует по avgTestStats.sharpeRatio (убывание). Затем последовательно выбирает из каждой группы по одному лучшему, пока не наберётся 50, но не превышая лимит на группу. Если в группе недостаточно инструментов – пропускает.
16. ReportExporter
Назначение	Формирует CSV-отчёт для МТ5.
Вход	const std::vector<SelectedInstrument>& topList
Выход	Файл top50_pairs.csv на диске.
Алгоритм	Открывает файл, открывает файл, записывает заголовок, для каждого инструмента выводит строку с символом, группой, параметрами и метриками.
Поток данных с учётом многопоточности (3 ядра)
Главный поток:

Запускает FileScanner > получает список инструментов (N штук).

Создаёт пул потоков размером 3.

Для каждого инструмента ставит задачу в пул:

Загрузить данные (DataLoader).

Построить таймфреймы (TimeFrameAggregator).

Определить группу (GroupClassifier).

Разбить на окна (WindowManager).

Для каждого окна последовательно (внутри одного потока) выполнить:

Получить уровни и тренд на train (через LevelDetector/TrendAnalyzer).

Извлечь признаки и обучить ML-модель (FeatureExtractor + MLPredictor).

Запустить оптимизацию (Optimizer) – перебор параметров, внутри каждого набора обучается модель и прогоняется бэктест на train.

Взять лучшие параметры и прогнать Evaluator на test.

Сохранить WindowResult.

Усреднить результаты по окнам > получить InstrumentResult.

Поместить в общий вектор (с мьютексом).

После завершения всех задач, в главном потоке вызывается TopSelector и ReportExporter.

Внутри Optimizer перебор параметров может быть дополнительно распараллелен, но при наличии 3 ядер и множества инструментов это не требуется – нагрузка распределяется на уровне инструментов.

Заключение
Представленная архитектура полностью покрывает требования:

Чтение бинарных файлов формата XFBAR.

Агрегация таймфреймов.

Реализация ТАП «Зри в корень» с оптимизацией параметров и ML-предсказанием.

Разбиение на 4-месячные окна (3 мес. обучение, 1 мес. тест).

Параллельная обработка на 3 ядрах.

Группировка по типам активов (спот, крипта, металлы) и отбор сбалансированного топ-50.

Экспорт результатов для МТ5.

Код может быть организован в виде одного main.cpp (с заголовочными файлами по желанию), использующего только STL (плюс std::thread). ML-модель пишется с нуля – это самодостаточно и не требует внешних зависимостей.