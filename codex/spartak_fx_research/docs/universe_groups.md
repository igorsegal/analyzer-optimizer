# Классификация доступного M5-universe

В `D:\AHexaTrader\DataFiles\raw` найдено 62 самостоятельных ценовых ряда M5, пригодных для первичного исследования. Классификация задаётся до оптимизации и не меняется после открытия OOS.

## Spot FX — 24

AUDJPY, AUDUSD, EURAUD, EURCAD, EURCHF, EURGBP, EURHUF, EURJPY, EURNOK, EURPLN, EURSEK, EURUSD, GBPJPY, GBPUSD, NZDUSD, USDCAD, USDCHF, USDCNH, USDHKD, USDJPY, USDMXN, USDNOK, USDSEK, USDSGD.

## Металлы — 5

XAGEUR, XAGUSD, XAUEUR, XAUUSD, XPDUSD.

## Крипто — 33

ADAUSD, APTUSD, ARBUSD, BCHUSD, BNBUSD, BTCUSD, CFXUSD, CROUSD, CRVUSD, DOTUSD, ETCUSD, ETHUSD, FILUSD, ICPUSD, INJUSD, JTOUSD, JUPUSD, LDOUSD, LTCUSD, PI-USD, POLUSD, SOLUSD, STXUSD, SUIUSD, TAOUSD, TIAUSD, TRXUSD, UNIUSD, WLDUSD, XLMUSD, XRPUSD, XTZUSD, ZROUSD.

## Что не вошло

- 392 каталога акций представлены только PERIOD_H1/D1, поэтому не участвуют в M5-версии системы;
- каталог EQIX пуст;
- файлы `_IND` содержат производные признаки и не считаются отдельными инструментами;
- один символ не должен попадать в лидерборд дважды через разные предварительно агрегированные таймфреймы.

## Правило топ-50

Для каждого из 62 инструментов на IS исследуется заранее зафиксированная сетка параметров. Один победитель на инструмент выбирается по validation после применения минимального числа сделок и прочих заранее заданных ограничений. Затем 62 победителя сортируются по единственной validation-метрике, первые 50 фиксируются вместе с параметрами и порядком. Blind OOS только проверяет эти 50 строк и не меняет состав или ранги.

Публиковать «топ-50 по OOS» нельзя: это превратит OOS в дополнительную оптимизационную выборку.
