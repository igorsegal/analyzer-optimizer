#include "core/InstrumentRegistry.h"
#include <algorithm>
#include <cctype>
namespace spartak::core {
const char* to_string(InstrumentCategory c) noexcept {
    switch (c) {
        case InstrumentCategory::Unknown:    return "Unknown";
        case InstrumentCategory::ForexMajor: return "ForexMajor";
        case InstrumentCategory::ForexCross: return "ForexCross";
        case InstrumentCategory::Metal:      return "Metal";
        case InstrumentCategory::Index:      return "Index";
        case InstrumentCategory::Crypto:     return "Crypto";
        case InstrumentCategory::Stock:      return "Stock";
    }
    return "?";
}
// Хелпер: заполнить base/quote из имени пары (XXXYYY)
static void fill_ccy(InstrumentSpec& s) {
    const auto& u = s.symbol;
    if (u.size() == 6 &&
        std::isalpha((unsigned char)u[0]) &&
        std::isalpha((unsigned char)u[3])) {
        s.base_currency  = u.substr(0, 3);
        s.quote_currency = u.substr(3, 3);
    }
}
// Хелпер: заполнить stop_buffer_points / trailing_distance_points по категории и масштабу цены.
// Логика: дистанции должны быть в единицах, сопоставимых с масштабом цены инструмента.
static void fill_distances(InstrumentSpec& s) {
    const auto& cat = s.category;
    // Для forex-пар масштаб одинаковый — используем базовые 30.
    if (cat == InstrumentCategory::ForexMajor || cat == InstrumentCategory::ForexCross) {
        s.stop_buffer_points       = 30;
        s.trailing_distance_points = 30;
        return;
    }
    // Металлы: золото в 1000x больше forex, серебро в 100x.
    if (cat == InstrumentCategory::Metal) {
        if (s.symbol.rfind("XAU", 0) == 0) {
            s.stop_buffer_points       = 300;
            s.trailing_distance_points = 300;
        } else if (s.symbol.rfind("XAG", 0) == 0) {
            s.stop_buffer_points       = 50;
            s.trailing_distance_points = 50;
        } else {
            s.stop_buffer_points       = 300;
            s.trailing_distance_points = 300;
        }
        return;
    }
    // Индексы: SP500 ~ 2000-4000, DE40 ~ 10000-15000, etc.
    if (cat == InstrumentCategory::Index) {
        const std::string& u = s.symbol;
        if (u == "SP500" || u == "NAS100" || u == "DJ30" || u == "US2000" ||
            u == "US30"  || u == "US500" || u == "HK50") {
            s.stop_buffer_points       = 100;
            s.trailing_distance_points = 100;
        } else if (u == "DE40" || u == "FRA40" || u == "EU50" || u == "UK100" ||
                   u == "SWI20" || u == "ES35" || u == "NETH25") {
            s.stop_buffer_points       = 300;
            s.trailing_distance_points = 300;
        } else if (u == "Nikkei225" || u == "CHINA50" || u == "CHINAH" ||
                   u == "HKTECH"   || u == "SPI200"  || u == "SA40"  ||
                   u == "SGP20"    || u == "TWINDEX" || u == "BVSPX") {
            s.stop_buffer_points       = 500;
            s.trailing_distance_points = 500;
        } else {
            s.stop_buffer_points       = 200;
            s.trailing_distance_points = 200;
        }
        return;
    }
    // Крипта: огромный масштаб цены.
    if (cat == InstrumentCategory::Crypto) {
        if (s.symbol.find("BTC") != std::string::npos) {
            s.stop_buffer_points       = 5000;
            s.trailing_distance_points = 5000;
        } else if (s.symbol.find("ETH") != std::string::npos) {
            s.stop_buffer_points       = 500;
            s.trailing_distance_points = 500;
        } else {
            s.stop_buffer_points       = 500;
            s.trailing_distance_points = 500;
        }
        return;
    }
    // Товары / прочее: средний масштаб.
    s.stop_buffer_points       = 100;
    s.trailing_distance_points = 100;
}
std::vector<InstrumentSpec>& InstrumentRegistry::storage() {
    static std::vector<InstrumentSpec> s = {
        { "EURUSD", InstrumentCategory::ForexMajor, 0.00001, 5, 100000.0, 0.01, 50.0, 0.01, 12, -7.2, -1.8, 5.0, 500.0 },
        { "GBPUSD", InstrumentCategory::ForexMajor, 0.00001, 5, 100000.0, 0.01, 50.0, 0.01, 15, -7.0, -2.0, 5.0, 500.0 },
        { "USDJPY", InstrumentCategory::ForexMajor, 0.001,   3, 100000.0, 0.01, 50.0, 0.01, 12, -5.5, -3.2, 5.0, 500.0 },
        { "USDCHF", InstrumentCategory::ForexMajor, 0.00001, 5, 100000.0, 0.01, 50.0, 0.01, 15, -6.5, -2.5, 5.0, 500.0 },
        { "AUDUSD", InstrumentCategory::ForexMajor, 0.00001, 5, 100000.0, 0.01, 50.0, 0.01, 15, -4.5, -4.0, 5.0, 500.0 },
        { "USDCAD", InstrumentCategory::ForexMajor, 0.00001, 5, 100000.0, 0.01, 50.0, 0.01, 15, -5.5, -3.0, 5.0, 500.0 },
        { "NZDUSD", InstrumentCategory::ForexMajor, 0.00001, 5, 100000.0, 0.01, 50.0, 0.01, 18, -4.0, -4.5, 5.0, 500.0 },
        { "EURGBP", InstrumentCategory::ForexCross, 0.00001, 5, 100000.0, 0.01, 50.0, 0.01, 18, -6.0, -3.0, 5.0, 500.0 },
        { "EURJPY", InstrumentCategory::ForexCross, 0.001,   3, 100000.0, 0.01, 50.0, 0.01, 18, -8.5, -2.0, 5.0, 500.0 },
        { "GBPJPY", InstrumentCategory::ForexCross, 0.001,   3, 100000.0, 0.01, 50.0, 0.01, 25, -9.0, -1.5, 5.0, 500.0 },
        { "XAUUSD", InstrumentCategory::Metal, 0.01,    2, 100.0,     0.01, 50.0, 0.01, 30, -12.0, -3.0, 7.0, 500.0 },
        { "XAGUSD", InstrumentCategory::Metal, 0.001,   3, 5000.0,    0.01, 50.0, 0.01, 25, -8.0, -2.0, 7.0, 500.0 },
        { "US500",  InstrumentCategory::Index, 0.1,     1, 1.0,       0.10, 50.0, 0.10, 50, -15.0, -5.0, 3.0, 200.0 },
        { "DE40",   InstrumentCategory::Index, 0.1,     1, 1.0,       0.10, 50.0, 0.10, 80, -15.0, -5.0, 3.0, 200.0 },
        { "BTCUSD", InstrumentCategory::Crypto, 0.01,   2, 1.0,       0.01, 10.0, 0.01, 500, -30.0, -10.0, 5.0, 100.0 },
        { "ETHUSD", InstrumentCategory::Crypto, 0.01,   2, 1.0,       0.01, 100.0, 0.01, 300, -25.0, -8.0, 5.0, 100.0 },
        { "BWXT",   InstrumentCategory::Stock, 0.01,    2, 1.0,       0.01, 1000.0, 0.01, 2, 0.0, 0.0, 1.0, 5.0 },
        { "AAPL",   InstrumentCategory::Stock, 0.01,    2, 1.0,       0.01, 1000.0, 0.01, 2, 0.0, 0.0, 1.0, 5.0 },
    };
    // Автозаполнение base/quote для forex-пар и крипты (по имени символа)
    static bool initialized = false;
    if (!initialized) {
        for (auto& spec : s) {
            const auto& u = spec.symbol;
            // Forex / крипта: XXXYYY  (6 символов, буквы)
            if (u.size() == 6 &&
                std::isalpha((unsigned char)u[0]) &&
                std::isalpha((unsigned char)u[3])) {
                // Пропускаем BTCUSDT (quote = USDT -> приравниваем к USD)
                if (u == "BTCUSDT" || u == "ETHUSDT" || u == "XRPUSDT" ||
                    u == "DOGEUSDT" || u == "ADAUSDT" || u == "SOLUSDT") {
                    spec.base_currency  = u.substr(0, u.size() - 4); // BTC, ETH...
                    spec.quote_currency = "USD";                      // USDT ~ USD
                } else {
                    spec.base_currency  = u.substr(0, 3);
                    spec.quote_currency = u.substr(3, 3);
                }
            }
            // Металлы и индексы — уже заданы выше через symbol, ставим USD
            else if (u.rfind("XAU", 0) == 0 || u.rfind("XAG", 0) == 0 ||
                     u.rfind("XPD", 0) == 0 || u.rfind("XPT", 0) == 0) {
                spec.base_currency  = u.substr(0, 3);
                spec.quote_currency = u.size() > 3 ? u.substr(3) : "USD";
            }
            else if (u == "SP500" || u == "NAS100" || u == "DJ30" || u == "US2000" ||
                     u == "US30"  || u == "US500"  || u == "HK50" || u == "CHINA50" ||
                     u == "CHINAH"|| u == "HKTECH" || u == "BVSPX" || u == "TWINDEX") {
                spec.base_currency  = u;
                spec.quote_currency = (u == "HK50" || u == "CHINAH" || u == "HKTECH") ? "HKD"
                                    : (u == "CHINA50" ? "CNH" : "USD");
            }
            else if (u == "DE40" || u == "FRA40" || u == "EU50" || u == "ES35" ||
                     u == "SWI20" || u == "NETH25") {
                spec.base_currency  = u;
                spec.quote_currency = "EUR";
            }
            else if (u == "UK100") {
                spec.base_currency = u; spec.quote_currency = "GBP";
            }
            else if (u == "Nikkei225" || u == "JP225") {
                spec.base_currency = u; spec.quote_currency = "JPY";
            }
            else if (u == "SPI200") {
                spec.base_currency = u; spec.quote_currency = "AUD";
            }
            else if (u == "SA40") {
                spec.base_currency = u; spec.quote_currency = "ZAR";
            }
            else if (u == "SGP20") {
                spec.base_currency = u; spec.quote_currency = "SGD";
            }
            // Товары
            else if (u == "UKOUSD" || u == "USOUSD" || u == "COPPER-C" ||
                     u == "GAS-C"   || u == "NG-C"    || u == "CL" ||
                     u == "Cocoa-C" || u == "Coffee-C"|| u == "Cotton-C" ||
                     u == "Sugar-C" || u == "Wheat-C" || u == "Soybean-C" ||
                     u == "OJ-C"    || u == "GASOIL-C") {
                spec.base_currency  = u;
                spec.quote_currency = "USD";
            }
        }
        initialized = true;
    }
    return s;
}
std::optional<InstrumentSpec>
InstrumentRegistry::lookup(const std::string& symbol) {
    for (const auto& s : storage()) {
        if (s.symbol.size() == symbol.size()) {
            bool eq = true;
            for (size_t i = 0; i < s.symbol.size(); ++i) {
                if (std::toupper(s.symbol[i]) != std::toupper(symbol[i])) { eq = false; break; }
            }
            if (eq) return s;
        }
    }
    return std::nullopt;
}
bool InstrumentRegistry::isForexMajor(const std::string& s) {
    static const char* majors[] = {"EURUSD","GBPUSD","USDJPY","USDCHF","AUDUSD","USDCAD","NZDUSD"};
    for (auto* m : majors) if (s == m) return true;
    return false;
}
bool InstrumentRegistry::isForexCross(const std::string& s) {
    static const char* ccy[] = {"EUR","GBP","JPY","CHF","AUD","CAD","NZD","USD"};
    if (s.size() != 6) return false;
    std::string a = s.substr(0, 3), b = s.substr(3, 3);
    bool ca = false, cb = false;
    for (auto* c : ccy) { if (a == c) ca = true; if (b == c) cb = true; }
    return ca && cb;
}
bool InstrumentRegistry::isMetal(const std::string& s) {
    return s.rfind("XAU", 0) == 0 || s.rfind("XAG", 0) == 0 ||
           s.rfind("XPT", 0) == 0 || s.rfind("XPD", 0) == 0;
}
bool InstrumentRegistry::isIndex(const std::string& s) {
    return s == "US500" || s == "DE40" || s == "UK100" ||
           s == "US30" || s == "NAS100" || s == "JP225";
}
bool InstrumentRegistry::isCrypto(const std::string& s) {
    return s.find("BTC") != std::string::npos ||
           s.find("ETH") != std::string::npos ||
           s.find("XRP") != std::string::npos ||
           s.find("LTC") != std::string::npos;
}
InstrumentSpec InstrumentRegistry::infer(const std::string& symbol) {
    InstrumentSpec s;
    s.symbol = symbol;
    std::string U = symbol;
    std::transform(U.begin(), U.end(), U.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    if (isForexMajor(U)) {
        if (auto exact = lookup(U)) return *exact;
    }
    if (isForexCross(U)) {
        s.category       = InstrumentCategory::ForexCross;
        s.point          = 0.00001;
        s.digits         = 5;
        s.contract_size  = 100000.0;
        s.leverage       = 500.0;
        s.spread_typical = 20;
        return s;
    }
    if (isMetal(U)) {
        s.category       = InstrumentCategory::Metal;
        s.point          = 0.01;
        s.digits         = 2;
        s.contract_size  = 100.0;
        s.leverage       = 500.0;
        s.spread_typical = 30;
        return s;
    }
    if (isIndex(U)) {
        s.category       = InstrumentCategory::Index;
        s.point          = 0.1;
        s.digits         = 1;
        s.contract_size  = 1.0;
        s.leverage       = 200.0;
        s.min_lot        = 0.10;
        s.lot_step       = 0.10;
        s.spread_typical = 50;
        return s;
    }
    if (isCrypto(U)) {
        s.category       = InstrumentCategory::Crypto;
        s.point          = 0.01;
        s.digits         = 2;
        s.contract_size  = 1.0;
        s.leverage       = 100.0;
        s.spread_typical = 300;
        return s;
    }
    s.category       = InstrumentCategory::Stock;
    s.point          = 0.01;
    s.digits         = 2;
    s.contract_size  = 1.0;
    s.leverage       = 5.0;
    s.min_lot        = 0.01;
    s.max_lot        = 1000.0;
    s.lot_step       = 0.01;
    s.spread_typical = 2;
    s.commission_per_lot = 1.0;
    s.swap_long_points   = 0.0;
    s.swap_short_points  = 0.0;
    return s;
}
InstrumentSpec InstrumentRegistry::resolve(const std::string& symbol) {
    InstrumentSpec s;
    if (auto exact = lookup(symbol)) s = *exact;
    else                             s = infer(symbol);
    fill_ccy(s);         // base/quote по имени
    fill_distances(s);   // stop/trail по категории
    return s;
}
void InstrumentRegistry::registerSpec(const InstrumentSpec& spec) {
    auto& v = storage();
    for (auto& s : v) {
        if (s.symbol == spec.symbol) { s = spec; return; }
    }
    v.push_back(spec);
}
std::vector<std::string> InstrumentRegistry::list() {
    std::vector<std::string> out;
    for (const auto& s : storage()) out.push_back(s.symbol);
    return out;
}
} // namespace spartak::core