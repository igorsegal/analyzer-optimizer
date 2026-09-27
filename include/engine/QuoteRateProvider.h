// =============================================================================
//  SPARTAK :: engine/QuoteRateProvider.h
//  Провайдер курсов валют к USD.
//
//  Задача: по имени инструмента и времени вернуть множитель конвертации
//  из валюты котировки в USD.
//
//    XXX/USD  ->  1.0
//    USD/XXX  ->  1 / price   (price = сколько XXX за 1 USD)
//    XXX/YYY  ->  требует данных по USDYYY
//
//  Поддерживается "стационарный" режим — одно значение курса на всю сессию
//  (среднее за период). Этого достаточно для PnL-конвертации в первом
//  приближении. Позже можно заменить на per-bar rate.
// =============================================================================
#pragma once
#include "core/InstrumentSpec.h"
#include <string>
#include <unordered_map>
namespace spartak::engine {
class QuoteRateProvider {
public:
    QuoteRateProvider() = default;
    // Установить курс валюты Q к USD: сколько USD за 1 единицу Q.
    // Пример: quote=JPY, rate=1/95 → 1 JPY = 0.0105 USD.
    void setRateToUsd(const std::string& quote_currency, double rate_to_usd);
    // Множитель для перевода PnL из валюты котировки в USD.
    // Принимает спеку инструмента и типичную цену (для USD/XXX).
    //
    //   XXX/USD: 1.0
    //   USD/XXX: 1/price
    //   XXX/YYY: rate[YYY]
    //   Если ничего не найдено: возвращает fallback (по умолчанию 1.0).
    double quoteToUsdFactor(const core::InstrumentSpec& spec,
                            double fallback_price) const;
    // Готовые пресеты для типовых валют.
    static QuoteRateProvider makeDefault();
private:
    std::unordered_map<std::string, double> rate_to_usd_;
};
} // namespace spartak::engine