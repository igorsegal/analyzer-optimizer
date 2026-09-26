# SPARTAK — ТЗ vs Код (аудит соответствия)
Дата: 2026-09-25
Назначение: сверить реализацию с исходным ТЗ «Зри в Корень» (ТАП)
Обозначения:
  ✅ = соответствует ТЗ
  ⚠️ = реализовано с отличием (числа/формула)
  ❌ = в ТЗ не было, добавлено мной
  🔴 = не реализовано, хотя было в ТЗ
---
## §1. ContextAnalyzer — ВХОДЫ/ВЫХОДЫ
### ТЗ
- Inputs: массивы MqlRates старших ТФ (Daily, 1Hour), параметры сглаживания/экстремумов
- Outputs:
  - Глобальный тренд (MarketTrend): Bullish (БТ), Bearish (МТ), Undefined (флэт)
  - Сетка уровней (LevelMap): ЛУ, ПУ + ОП, ОС
### Код
- context/ContextAggregator.h: `analyze(older_tf, younger_tf)` -> `MarketContext`
- MarketContext: `daily_trend`, `hourly_trend`, `dominant_trend`, `active_zones`
### Статус: ✅ (структурно)
- ⚠️ В BacktestPlayer сейчас `use_same_tf_for_both = true` — оба ТФ подаются как M5. **Требует исправления** для multi-TF.
---
## §2.1. Тренд БТ (бычий)
### ТЗ
1. Правило экстремумов: `High_i > High_{i-2}` И `Low_i > Low_{i-2}`
2. Импульсный компонент (ИК-БТ): резкий рост от минимума к максимуму
3. Коррекционный (ККТ-БТ): откат, не нарушающий восходящий минимум
### Код
- context/StructureValidatorHHHL::validate()
  - Проверяет последовательность из `lookback` (дефолт 3) последних High-экстремумов и Low-экстремумов
  - Сравнение соседних High: `h[k] > h[k-1]`
  - Сравнение соседних Low: `l[k] > l[k-1]`
### Статус: ⚠️
- По сути эквивалент ТЗ, если считать что `High_i` и `High_{i-2}` — это соседние High в общем массиве H-L-H-L. Моя реализация выделяет High/Low в отдельные массивы и сравнивает соседние — логически то же.
- ❗ НЕ реализовано: **разделение на ИК (импульс) и ККТ (коррекция)**. Только правило экстремумов.
---
## §2.2. Тренд МТ (медвежий)
### ТЗ
1. `Low_i < Low_{i-2}` И `High_i < High_{i-2}`
2. Аналогично БТ
### Код
- Аналогично симметрично в StructureValidatorHHHL
### Статус: ⚠️ (такое же — эквивалент, ИК/ККТ не разделены)
---
## §3.1. Классификация уровней
### ТЗ
- **ЛУ**: экстремум на фрактальной структуре младшего ТФ (H1)
- **ПУ**: ключевой уровень старшего ТФ (Daily), барьер для глобального тренда
- **ОП**: диапазон снизу, где был интерес покупателей
- **ОС**: диапазон сверху, где было давление продавцов
### Код
- context/PUZoneCalculator.cpp: строит PriceZone с `LevelType::IntermediateLevel`
- context/LUZoneCalculator.cpp: строит PriceZone с `LevelType::LocalLevel`
- Обе используют формулу `zone = [price - offset*point, price + offset*point]`
### Статус: ✅
- ⚠️ В BacktestPlayer сейчас **оба калькулятора получают экстремумы одного и того же M5**. Должно быть: PU — из Daily, LU — из H1. **Требует исправления в BacktestPlayer**.
---
## §3.2. Формула зон
### ТЗ
- `Zone_top = P_extreme + (offset_multiplier × Point)`
- `Zone_bottom = P_extreme - (offset_multiplier × Point)`
- `offset_multiplier`: 10–20 пунктов
### Код
- PUZoneCalculator: `offset_points = 15` (в диапазоне)
- LUZoneCalculator: `offset_points = 10` (в диапазоне)
### Статус: ✅
---
## §PatternDetector — Ложный пробой
### ТЗ
- Хвост свечи прокалывает зону
- Тело закрывается выше/ниже уровня
- **Длина проколовшей тени ≥ 30% от всего диапазона (High - Low)**
### Код
- patterns/PinbarBuyDetector.cpp / PinbarSellDetector.cpp
- Дефолт `wick_ratio_min = 0.25` (25%)
### Статус: 🔴 **Нарушение!**
- **Должно быть 30%**. Сейчас 25%. Патч обязателен.
---
## §PatternDetector — Закрепление (Consolidation)
### ТЗ
- Удержание уровня серией из **2-3 баров**
### Код
- patterns/InsideBarDetector.cpp — детект одной свечи внутри предыдущей
- patterns/PatternAggregator.cpp — Consolidation реализован как **InsideBar + тренд**, а не как «удержание уровня N баров»
### Статус: 🔴 **Не реализовано**
- В ТЗ: серия 2-3 закрытий по одну сторону от уровня
- В коде: одиночный InsideBar
- Для «серии 2-3» есть готовый модуль `patterns/BarCloseConfirmationCounter.cpp` — но он **не используется в PatternAggregator**. Требуется интеграция.
---
## §PatternDetector — Импульсный прорыв
### ТЗ
- Закрытие свечи за пределами зоны с крупным телом
### Код
- patterns/ATRImpulseBreakoutDetector.cpp
- Дефолт: `body_ratio >= 0.60`, `close_position >= 0.75`, `range >= 1.5 × ATR`
### Статус: ⚠️
- ❌ **ATR-фильтр (`range >= 1.5×ATR`) в ТЗ НЕ БЫЛ** — я придумал.
- ✅ Остальное соответствует.
- Решение: оставить или убрать? ATR-фильтр отсеивает слабые импульсы, но может рубить полезные. **Нужно решение пользователя.**
---
## §SignalValidator — Фильтр спреда
### ТЗ
- Если спред в пунктах превышает `max_allowed_spread`, ордер отклоняется
### Код
- validation/SpreadFilter.cpp
- Дефолт `max_points = 25`
### Статус: ✅
---
## §SignalValidator — Фильтр тренда
### ТЗ
- BUY разрешён только при Bullish тренде старшего ТФ
- SELL разрешён только при Bearish
- **Исключение:** контр-тренд разрешён, если паттерн сформирован на ПУ (IntermediateLevel)
### Код
- validation/CounterTrendPUChecker.cpp
- Флаг `allow_counter_trend_on_pu = true`
- Проверяет: signal.level на активном ПУ (по eps 2 pts)
### Статус: ✅
---
## §SignalValidator — Lot Sizing
### ТЗ
- Расчёт объёма от % риска и расстояния до стоп-лосса
### Код
- validation/MoneyRiskCalculator.cpp
- Формула: `risk_money / (stop_dist × contract_size)`
### Статус: ✅
---
## §SignalValidator — Нормализация лота
### ТЗ
- Округление до шага 0.01, минимум 0.01
### Код
- validation/BrokerLotRounder.cpp
- floor до шага, отсечка min/max
### Статус: ✅
---
## §SignalValidator — Take Profit
### ТЗ
- **TP = 2:1 к риску** (если сигнал не задал свой)
### Код
- validation/SignalValidator.cpp
- `tp_risk_ratio = 2.0`
- TP1 = 1:1, TP2 = 2:1
### Статус: ⚠️
- ✅ TP2 = 2:1 — соответствует ТЗ
- ❌ **TP1 = 1:1 — НЕ было в ТЗ**. Это я придумал.
- В ТЗ только **один** TP = 2:1. Идея TP1 появляется только в PositionManager (см. ниже).
---
## §PositionManager — TP1 → PartialClose
### ТЗ
- TP1 = первая цель
- При достижении: **PartialClose 50% от первоначального объёма**
- Объём округлить с учётом min_lot
### Код
- position/UO1Trigger.cpp — детект TP1/TP2/SL
- position/VolumeSplitter50.cpp — split 50/50
- position/PositionManager.cpp — вызывает split при TP1
### Статус: ✅
---
## §PositionManager — Мультисплит 50 / 25 / 25
### ТЗ
- **Первый абзац ТЗ PositionManager:** «Логика мультисплит-выхода: возможность частичного закрытия позиции частями (сначала 50%, затем 25% и еще 25% от первоначального объема)»
### Код
- VolumeSplitter50 даёт **одно** закрытие 50%. Остаток 50% держится до TP2/SL.
- Ступени 25% + 25% **НЕ реализованы**.
### Статус: 🔴 **Не реализовано**
- Требуется: после TP1 (50%) должны быть ещё две цели TP-1.5 и TP2 (по 25% каждая).
- **Открытый вопрос:** где эти цели? 1.5:1 и 2:1? Или на других уровнях?
---
## §PositionManager — MoveBreakEven
### ТЗ
- Стоп переносится на цену входа с поправкой на спред брокера, после TP1
### Код
- position/BreakEvenTransfer.cpp
- `new_sl = entry + spread × point` для BUY
- `be_clamp_buffer_points = 1` (защита от перепрыгивания TP1)
### Статус: ✅
---
## §PositionManager — FullClose
### ТЗ
- FullClose по SL, TP2, ручное
### Код
- position/PositionManager.cpp — `closeFull(...)` с reason "sl"/"tp2"/...
### Статус: ✅
---
## §Позиции, добавленные мной (не было в ТЗ)
### ❌ position/TrailingStopManager — трейлинг-стоп
- В ТЗ PositionManager **не упомянут**
- Мой add-on: трейлинг только после TP1, расстояние 50 pts
### ❌ position/EmergencyCloseHandler — аварийное закрытие
- Не было в ТЗ
- Мой add-on: margin call, drawdown, max holding days
### ❌ engine/BacktestPlayer — cooldown между сделками
- Не было в ТЗ
- Предложил в v4: не открывать N баров после закрытия
### ❌ patterns/ATRImpulseBreakout — ATR-фильтр
- В ТЗ: только «крупное тело»
- Мой add-on: `range >= 1.5×ATR`
### ❌ patterns/PatternAggregator — приоритет паттернов
- В ТЗ: три типа РМ (ложный пробой, закрепление, импульс)
- В коде: жёсткий приоритет FalseBreakout > Impulse > Consolidation
- В ТЗ не описано как разрешать коллизии
---
## СВОДКА
| # | Пункт | Статус |
|---|---|---|
| 1 | Экстремумы: формула | ⚠️ эквивалент |
| 2 | ИК / ККТ разделение | 🔴 не реализовано |
| 3 | Зона PU/LU формула | ✅ |
| 4 | PU/LU на разных ТФ | 🔴 BacktestPlayer использует один ТФ |
| 5 | Wick ratio 30% | 🔴 стоит 25% |
| 6 | Consolidation: серия 2-3 баров | 🔴 реализовано как InsideBar |
| 7 | Impulse ATR-фильтр | ❌ мой add-on |
| 8 | SpreadFilter | ✅ |
| 9 | Counter-trend PU exception | ✅ |
| 10 | Lot Sizing | ✅ |
| 11 | Нормализация | ✅ |
| 12 | TP 2:1 | ✅ |
| 13 | TP1 = 1:1 | ❌ мой add-on |
| 14 | TP1 → PartialClose 50% | ✅ |
| 15 | **Мультисплит 50/25/25** | 🔴 не реализовано |
| 16 | MoveBreakEven с спредом | ✅ |
| 17 | FullClose по SL/TP2 | ✅ |
| 18 | Trailing | ❌ мой add-on |
| 19 | Emergency close | ❌ мой add-on |
| 20 | Cooldown | ❌ мой add-on |
| 21 | Multi-TF в BacktestPlayer | 🔴 не реализовано |
### Критические (🔴) — требуют правки для соответствия ТЗ:
1. Multi-TF в BacktestPlayer (пункт 4, 21)
2. Wick ratio 25% → 30% (пункт 5)
3. Consolidation через BarCloseConfirmationCounter (пункт 6)
4. Мультисплит 50/25/25 (пункт 15)
5. ИК/ККТ разделение (пункт 2) — большой вопрос
### Добавленные сверх ТЗ (❌) — решить: оставить / убрать:
- Trailing stop
- Emergency close
- Cooldown
- ATR-фильтр
- TP1 = 1:1
---
## ВОПРОСЫ К ПОЛЬЗОВАТЕЛЮ
1. Пункт 1 (экстремумы): ТЗ говорит `High_i > High_{i-2}`. Считать ли что моя реализация (сравнение соседних High в отдельном массиве) соответствует? Или в ТЗ имелось в виду "через один"?
2. Пункт 2 (ИК / ККТ): нужно ли реализовывать? В ТЗ упомянуто, но не формализовано как отдельный критерий.
3. Пункт 6 (Consolidation): правильно ли понимаю, что нужно использовать BarCloseConfirmationCounter (2-3 бара подряд закрытий с одной стороны)?
4. Пункт 15 (мультисплит): где две промежуточные цели? Пропорционально (1.33:1 и 1.66:1) или на других уровнях? Или TP1 = 1:1 (50%), TP2 = 2:1 (25%), TP3 = 3:1 (25%)?
5. Пункты ❌: что из этого оставить, что убрать?