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
    if (auto exact = lookup(symbol)) return *exact;
    return infer(symbol);
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