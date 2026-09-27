#include "engine/QuoteRateProvider.h"
#include <stdexcept>
namespace spartak::engine {
void QuoteRateProvider::setRateToUsd(const std::string& quote_currency, double rate_to_usd) {
    if (rate_to_usd <= 0.0)
        throw std::invalid_argument("QuoteRateProvider: rate must be > 0");
    rate_to_usd_[quote_currency] = rate_to_usd;
}
double QuoteRateProvider::quoteToUsdFactor(const core::InstrumentSpec& spec,
                                           double fallback_price) const {
    const auto& q = spec.quote_currency;
    // 1. Если котировка уже USD -> 1.0
    if (q == "USD" || q == "USDT") return 1.0;
    // 2. Если база USD, а котировка XXX: USD/XXX (например, USDJPY)
    //    PnL в XXX, курс = price (сколько XXX за 1 USD), значит 1 XXX = 1/price USD.
    if (spec.base_currency == "USD") {
        if (fallback_price > 0.0) return 1.0 / fallback_price;
        // Если нет цены — берём из таблицы как (1/price_XXX)
        auto it = rate_to_usd_.find(q);
        if (it != rate_to_usd_.end()) return it->second;
        return 1.0;
    }
    // 3. Cross-пара XXX/YYY: нужен курс YYY -> USD
    auto it = rate_to_usd_.find(q);
    if (it != rate_to_usd_.end()) return it->second;
    // 4. Не нашли — fallback
    return 1.0;
}
QuoteRateProvider QuoteRateProvider::makeDefault() {
    QuoteRateProvider p;
    // Типичные средние курсы к USD за 2013-2019.
    p.setRateToUsd("USD", 1.0);
    p.setRateToUsd("USDT", 1.0);
    p.setRateToUsd("EUR", 1.15);
    p.setRateToUsd("GBP", 1.35);
    p.setRateToUsd("JPY", 1.0 / 110.0);   // 1 JPY ~ 0.0091 USD
    p.setRateToUsd("CHF", 1.02);
    p.setRateToUsd("AUD", 0.75);
    p.setRateToUsd("CAD", 0.78);
    p.setRateToUsd("NZD", 0.70);
    p.setRateToUsd("CNH", 0.145);
    p.setRateToUsd("HKD", 0.128);
    p.setRateToUsd("SGD", 0.74);
    p.setRateToUsd("SEK", 0.115);
    p.setRateToUsd("NOK", 0.118);
    p.setRateToUsd("DKK", 0.154);
    p.setRateToUsd("PLN", 0.26);
    p.setRateToUsd("TRY", 0.25);
    p.setRateToUsd("MXN", 0.052);
    p.setRateToUsd("ZAR", 0.072);
    p.setRateToUsd("CZK", 0.044);
    p.setRateToUsd("HUF", 0.0036);
    p.setRateToUsd("INR", 0.015);
    p.setRateToUsd("KRW", 0.00088);
    p.setRateToUsd("BRL", 0.27);
    p.setRateToUsd("CLP", 0.0015);
    p.setRateToUsd("COP", 0.00032);
    p.setRateToUsd("IDR", 0.00007);
    p.setRateToUsd("ILS", 0.28);
    p.setRateToUsd("THB", 0.031);
    p.setRateToUsd("TWD", 0.032);
    return p;
}
} // namespace spartak::engine