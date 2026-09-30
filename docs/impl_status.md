# Состояние реализации ТАП «Зри в Корень»
Дата: 2026-09-30
Источник требований: `docs/tz/zri_v_koren.md` (PDF/PPTX).
## Реализовано
| # | Требование ТЗ | Файл в коде | Примечание |
|---|---|---|---|
| 1 | Тренд Daily + 1H | context/TrendBiasEvaluator | через StructureValidatorHHHL |
| 2 | Структура HH/HL | context/StructureValidatorHHHL | правило i vs i-2 |
| 3 | ЛУ — локальный уровень | context/LUZoneCalculator | offset 10 pts |
| 4 | ПУ — промежуточный уровень | context/PUZoneCalculator | offset 15 pts |
| 5 | Зоны ОП / ОС | core/Types.h PriceZone | через is_active |
| 6 | РМ: закрепление | patterns/InsideBarDetector | одна свеча |
| 7 | РМ: ложный пробой | patterns/PinbarBuyDetector / PinbarSellDetector | pinbar |
| 8 | РМ: поглощение | patterns/EngulfingDetector | не было в ТЗ явно |
| 9 | РМ: прорыв | patterns/ATRImpulseBreakoutDetector | ATR-фильтр |
| 10 | ТВ в области ПУ/ЛУ | patterns/PatternAggregator | через active_zones |
| 11 | Стоп с учётом теней | stop_buffer_points | per-instrument |
| 12 | Сопровождение 50% / 50% | position/VolumeSplitter50 | partial_pcts {0.50, 0.25, 0.25} |
| 13 | Перенос стопа в УБ | position/BreakEvenTransfer | с учётом спреда |
| 14 | Фильтр спреда | validation/SpreadFilter | absolute + avg |
| 15 | Расчёт лота от риска | validation/MoneyRiskCalculator | + commission iter |
| 16 | Проверка маржи | validation/MarginCallChecker | min_margin_level |
| 17 | Сессионный фильтр | validation/SessionTimeFilter | UTC |
## Не реализовано
| # | Требование ТЗ | Текущее состояние | Что нужно |
|---|---|---|---|
| 1 | Multi-TF Daily + 1H + M5-M15 раздельно | ContextAggregatorConfig.use_same_tf_for_both = true | Читать 3 .bin, синхронизировать по времени |
| 2 | Четыре вида РМ как отдельные типы | В коде 4 паттерна, но без привязки к видам РМ из ТЗ | Маппинг: InsideBar = закрепление; Pinbar/Engulfing = закрепление/пробой; ATRImpulse = прорыв |
| 3 | ИКТ / ККТ (импульс / коррекция тенденции) | Отсутствует | Добавить разделитель участков тренда |
| 4 | ЛУ/ПУ, ближайшие к цене | Берутся последние N экстремумов | Сортировать по |price - current_close| |
| 5 | Правила ТВ как явная проверка 4 пунктов | Разрознено в PatternAggregator + SignalValidator | Свести в один валидатор TV BUY / TV SELL |
| 6 | Сопровождение строго 50/50 (без трейла) | VolumeSplitter50 + TrailingStopManager | Решить: трейл оставляем или убираем |
## Дополнительно в коде (сверх ТЗ)
| # | Что | Файл | Решение |
|---|---|---|---|
| 1 | Trailing stop | position/TrailingStopManager | Открыто: оставить или убрать |
| 2 | Emergency close | position/EmergencyCloseHandler | Открыто: оставить или убрать |
| 3 | ATR-фильтр в импульсе | patterns/ATRImpulseBreakoutDetector | Открыто: оставить или убрать |
| 4 | TP1 = 1:1 | validation/SignalValidator | В ТЗ только TP2 |
| 5 | Engulfing detector | patterns/EngulfingDetector | В ТЗ явно не было |
## Что решить после видео
1. Длина тени ложного пробоя — 25% / 30% / 60%? В PDF не указано.
2. Мультисплит 50/25/25 или 50/50? PDF стр. 20 показывает 50/50.
3. Закрепление — 1 свеча или серия? PDF не уточняет.
4. Числовые критерии ИК / ККТ.
5. Ближайшие ЛУ/ПУ — по какой метрике? (расстояние в pts / ATR / %)
## Как продолжить
1. Открыть этот файл.
2. Ответить на 5 вопросов из раздела «Что решить после видео».
3. Обновить разделы «Не реализовано» и «Дополнительно».
4. Взяться за пункт 1 (Multi-TF) или за оптимизацию порога пробоя — по выбору.