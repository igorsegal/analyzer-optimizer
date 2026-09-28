// =============================================================================
//  SPARTAK :: tools/portfolio_runner.cpp
// =============================================================================
#include "engine/PortfolioBacktest.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <tuple>
#include <map>
#include <algorithm>
#include <filesystem>
#include <cstdio>
#include <cstdint>
#include <cctype>

namespace fs = std::filesystem;
using namespace spartak;

struct IniConfig {
    std::map<std::string, std::string> kv;
    std::string get_s(const std::string& key, const std::string& def = "") const {
        auto it = kv.find(key);
        return it == kv.end() ? def : it->second;
    }
    double get_d(const std::string& key, double def) const {
        auto it = kv.find(key);
        if (it == kv.end() || it->second.empty()) return def;
        try { return std::stod(it->second); } catch (...) { return def; }
    }
    bool get_b(const std::string& key, bool def) const {
        auto it = kv.find(key);
        if (it == kv.end()) return def;
        std::string v;
        for (char c : it->second) v.push_back((char)std::tolower(c));
        if (v == "true" || v == "yes" || v == "1" || v == "on")  return true;
        if (v == "false"|| v == "no"  || v == "0" || v == "off") return false;
        return def;
    }
    std::vector<std::string> get_csv(const std::string& key) const {
        std::vector<std::string> out;
        auto it = kv.find(key);
        if (it == kv.end()) return out;
        std::stringstream ss(it->second);
        std::string tok;
        while (std::getline(ss, tok, ',')) {
            size_t a = tok.find_first_not_of(" \t");
            size_t b = tok.find_last_not_of(" \t");
            if (a == std::string::npos) continue;
            out.push_back(tok.substr(a, b - a + 1));
        }
        return out;
    }
};

static bool load_ini(const std::string& path, IniConfig& out) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t a = line.find_first_not_of(" \t");
        if (a == std::string::npos) continue;
        if (line[a] == '#' || line[a] == ';') continue;
        size_t eq = line.find('=', a);
        if (eq == std::string::npos) continue;
        std::string k = line.substr(a, eq - a);
        std::string v = line.substr(eq + 1);
        size_t ka = k.find_first_not_of(" \t");
        size_t kb = k.find_last_not_of(" \t");
        if (ka == std::string::npos) continue;
        k = k.substr(ka, kb - ka + 1);
        size_t va = v.find_first_not_of(" \t");
        size_t vb = v.find_last_not_of(" \t");
        v = (va == std::string::npos) ? "" : v.substr(va, vb - va + 1);
        out.kv[k] = v;
    }
    return true;
}

static int64_t parse_date_utc(const std::string& s) {
    int y = 0, mo = 0, d = 0;
    if (std::sscanf(s.c_str(), "%d-%d-%d", &y, &mo, &d) != 3) return 0;
    auto days_from_civil = [](int yy, unsigned mm, unsigned dd) -> int64_t {
        yy -= (mm <= 2);
        const int era = (yy >= 0 ? yy : yy - 399) / 400;
        const unsigned yoe = static_cast<unsigned>(yy - era * 400);
        const unsigned doy = (153 * (mm + (mm > 2 ? -3 : 9)) + 2) / 5 + dd - 1;
        const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468;
    };
    return days_from_civil(y, static_cast<unsigned>(mo), static_cast<unsigned>(d)) * 86400LL * 1000LL;
}

enum class Cat { FX, Metal, Crypto, Skip };

struct UniverseFilter {
    std::set<std::string> fx_majors;
    std::set<std::string> crypto_bases;
    std::set<std::string> metals;
    std::set<std::string> blacklist;
    bool jpy_pairs = false;
    bool fx_major  = true;
    bool fx_cross  = true;
    bool metals_on = true;
    bool crypto_on = true;

    Cat classify(const std::string& s) const {
        if (blacklist.count(s)) return Cat::Skip;
        if (!jpy_pairs) {
            if (s.size() == 6 && (s.substr(3,3) == "JPY" || s.substr(0,3) == "JPY"))
                return Cat::Skip;
        }
        if (metals_on && metals.count(s)) return Cat::Metal;
        if (crypto_on) {
            for (auto& p : crypto_bases)
                if (s == p + "USD" || s == p + "USDT") return Cat::Crypto;
        }
        if (s.size() == 6) {
            auto b = s.substr(0, 3), q = s.substr(3, 3);
            if (fx_majors.count(b) && fx_majors.count(q) && b != q) {
                const bool is_major = (b == "USD" || q == "USD");
                if (is_major && fx_major)  return Cat::FX;
                if (!is_major && fx_cross) return Cat::FX;
            }
        }
        return Cat::Skip;
    }
};

static const char* cat_name(Cat c) {
    switch (c) {
        case Cat::FX:     return "FX";
        case Cat::Metal:  return "METAL";
        case Cat::Crypto: return "CRYPTO";
        default:          return "?";
    }
}

static const char* pattern_name(int t) {
    switch (t) {
        case 0: return "None";
        case 1: return "FalseBreakout";
        case 2: return "Consolidation";
        case 3: return "ImpulseBreakout";
        default: return "?";
    }
}

static const char* side_name(int s) { return (s == 0) ? "Buy" : "Sell"; }

static void discover(const fs::path& root,
                     const std::string& tf_suffix,
                     const UniverseFilter& flt,
                     std::vector<std::string>& files,
                     std::vector<std::string>& syms,
                     std::vector<Cat>&         cats)
{
    std::vector<std::tuple<std::string, std::string, Cat>> rows;
    for (auto& e : fs::directory_iterator(root)) {
        if (!e.is_directory()) continue;
        std::string sym = e.path().filename().string();
        Cat c = flt.classify(sym);
        if (c == Cat::Skip) continue;
        fs::path f = e.path() / (sym + tf_suffix);
        if (!fs::exists(f)) continue;
        rows.emplace_back(sym, f.string(), c);
    }
    std::sort(rows.begin(), rows.end(),
              [](const auto& a, const auto& b) {
                  return std::get<0>(a) < std::get<0>(b);
              });
    for (auto& [s, p, c] : rows) {
        syms.push_back(s);
        files.push_back(p);
        cats.push_back(c);
    }
}

static void print_report(const engine::PortfolioReport& r) {
    std::cout << "\n";
    std::cout << "===============================================================\n";
    std::cout << " SPARTAK PORTFOLIO REPORT\n";
    std::cout << "===============================================================\n";
    std::cout << " Instruments loaded  : " << r.instruments_loaded << "\n";
    std::cout << " Instruments failed  : " << r.instruments_failed << "\n";
    std::cout << " Bars processed      : " << r.bars_processed << "\n";
    std::cout << " Signals total       : " << r.signals_total << "\n";
    std::cout << " Signals skipped     : " << r.signals_skipped_type << "\n";
    std::cout << " Orders approved     : " << r.orders_approved << "\n";
    std::cout << " Orders rejected     : " << r.orders_rejected << "\n";
    std::cout << "   margin blocked    : " << r.orders_margin_blocked << "\n";
    std::cout << "   rr < min_rr       : " << r.reject_rr << "\n";
    std::cout << " Partial closes      : " << r.partial_closes << "\n";
    std::cout << " Full closes         : " << r.full_closes << "\n";
    std::cout << "\n";
    std::cout << " Closed trades       : " << (r.wins + r.losses) << "\n";
    std::cout << "   wins              : " << r.wins << "\n";
    std::cout << "   losses            : " << r.losses << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << " Win rate            : " << r.win_rate      << " %\n";
    std::cout << " Profit factor       : " << r.profit_factor << "\n";
    std::cout << "\n";
    std::cout << " Initial balance     : $" << r.initial_balance << "\n";
    std::cout << " Final balance       : $" << r.final_balance   << "\n";
    std::cout << " Net PnL             : $" << r.net_pnl         << "\n";
    std::cout << " Return              : " << r.return_pct      << " %\n";
    std::cout << " Peak equity         : $" << r.peak_equity     << "\n";
    std::cout << " Max drawdown (equity): " << r.max_drawdown_pct << " %\n";
    std::cout << "===============================================================\n";

    std::cout << "\nDaily drawdown (equity):\n";
    std::cout << " Worst day           : " << r.worst_day
              << " (" << r.max_daily_dd_pct << " %)\n";

    std::cout << "\nMonthly (by closed trades):\n";
    std::cout << " Months total        : " << r.total_months << "\n";
    std::cout << "   positive          : " << r.months_positive << "\n";
    std::cout << "   negative          : " << r.months_negative << "\n";
    std::cout << "   flat              : " << r.months_flat << "\n";
    if (r.total_months > 0) {
        std::cout << " Worst month         : $" << r.worst_month_usd
                  << "  (" << r.worst_month_pct << " %)\n";
        std::cout << " Best month          : $" << r.best_month_usd
                  << "  (" << r.best_month_pct << " %)\n";
        std::cout << " Avg month           : $" << r.avg_month_usd
                  << "  (" << r.avg_month_pct << " %)\n";
    }

    if (!r.per_year.empty()) {
        std::cout << "\nPer-year:\n";
        std::cout << "  " << std::left << std::setw(6) << "Year"
                  << std::setw(8) << "Orders"
                  << std::setw(8) << "Wins"
                  << std::setw(8) << "Losses"
                  << std::setw(10) << "WR%"
                  << std::setw(14) << "PF"
                  << "  Net PnL\n";
        for (const auto& y : r.per_year) {
            const std::size_t tot = y.wins + y.losses;
            const double wr = (tot > 0) ? (double)y.wins / tot * 100.0 : 0.0;
            const double pf = (y.total_loss > 0.0)
                            ? y.total_win / y.total_loss
                            : (y.total_win > 0.0 ? 999.0 : 0.0);
            std::cout << "  " << std::left << std::setw(6) << y.year
                      << std::setw(8) << y.orders
                      << std::setw(8) << y.wins
                      << std::setw(8) << y.losses
                      << std::setw(10) << std::setprecision(2) << wr
                      << std::setw(14) << pf
                      << "  $" << y.net_pnl << "\n";
        }
    }

    if (!r.per_pattern.empty()) {
        std::cout << "\nPer-pattern:\n";
        std::cout << "  " << std::left << std::setw(20) << "Pattern"
                  << std::setw(6) << "Side"
                  << std::setw(8) << "Orders"
                  << std::setw(8) << "Wins"
                  << std::setw(8) << "Losses"
                  << std::setw(10) << "WR%"
                  << std::setw(14) << "PF"
                  << "  Net PnL\n";
        for (const auto& p : r.per_pattern) {
            const std::size_t tot = p.wins + p.losses;
            const double wr = (tot > 0) ? (double)p.wins / tot * 100.0 : 0.0;
            const double pf = (p.total_loss > 0.0)
                            ? p.total_win / p.total_loss
                            : (p.total_win > 0.0 ? 999.0 : 0.0);
            std::cout << "  " << std::left << std::setw(20)
                      << pattern_name(p.type)
                      << std::setw(6) << side_name(p.side)
                      << std::setw(8) << p.orders
                      << std::setw(8) << p.wins
                      << std::setw(8) << p.losses
                      << std::setw(10) << std::setprecision(2) << wr
                      << std::setw(14) << pf
                      << "  $" << p.net_pnl << "\n";
        }
    }
}

static void print_usage() {
    std::cout <<
        "SPARTAK Portfolio Runner\n"
        "Usage:\n"
        "  portfolio_runner.exe --config <path> [--dir <override>] [--list]\n";
}

int main(int argc, char** argv) {
    std::string config_path = "spartak.ini";
    std::string raw_dir_override;
    bool list_only = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if      (a == "--config" && i + 1 < argc) config_path = argv[++i];
        else if (a == "--dir"    && i + 1 < argc) raw_dir_override = argv[++i];
        else if (a == "--list")                   list_only = true;
        else if (a == "--help" || a == "-h") { print_usage(); return 0; }
        else { std::cerr << "Unknown argument: " << a << "\n"; print_usage(); return 1; }
    }

    IniConfig ini;
    if (!load_ini(config_path, ini)) {
        std::cerr << "Error: cannot open config: " << config_path << "\n";
        return 1;
    }

    const std::string tf = ini.get_s("timeframe", "H1");
    const std::string tf_suffix = "_" + tf + ".bin";

    std::size_t agg = 1;
    if      (tf == "M5")  agg = 12;
    else if (tf == "M15") agg = 4;
    else if (tf == "M30") agg = 2;
    else                  agg = 1;

    engine::PortfolioConfig cfg;
    cfg.initial_balance         = ini.get_d("balance",          10'000.0);
    cfg.risk_percent            = ini.get_d("risk_percent",     2.5);
    cfg.min_margin_level_pct    = ini.get_d("min_margin",       500.0);
    cfg.min_rr                  = ini.get_d("min_rr",           1.34);
    cfg.spread_mult             = ini.get_d("spread_mult",      1.0);
    cfg.commission_mult         = ini.get_d("commission_mult",  1.0);
    cfg.compound_sizing         = ini.get_b("compound",         false);
    cfg.skip_false_breakout     = ini.get_b("skip_false_breakout",   true);
    cfg.skip_impulse_buy        = ini.get_b("skip_impulse_buy",      true);
    cfg.skip_impulse_sell       = ini.get_b("skip_impulse_sell",     true);
    cfg.skip_consolidation_buy  = ini.get_b("skip_consolidation_buy", true);
    cfg.skip_consolidation_sell = ini.get_b("skip_consolidation_sell", false);
    cfg.aggregate_bars          = agg;
    cfg.from_ms = parse_date_utc(ini.get_s("from_date", ""));
    cfg.to_ms   = parse_date_utc(ini.get_s("to_date",   ""));

    std::string raw_dir = raw_dir_override.empty()
                        ? ini.get_s("raw_dir", "D:/AHexaTrader/1DataFiles/raw")
                        : raw_dir_override;

    UniverseFilter flt;
    for (auto& s : ini.get_csv("fx_currencies"))   flt.fx_majors.insert(s);
    for (auto& s : ini.get_csv("crypto_bases"))    flt.crypto_bases.insert(s);
    for (auto& s : ini.get_csv("metals"))          flt.metals.insert(s);
    for (auto& s : ini.get_csv("blacklist"))       flt.blacklist.insert(s);
    flt.jpy_pairs = ini.get_b("jpy_pairs", false);
    flt.fx_major  = ini.get_b("fx_major",  true);
    flt.fx_cross  = ini.get_b("fx_cross",  true);
    flt.metals_on = ini.get_b("metals_on", true);
    flt.crypto_on = ini.get_b("crypto_on", true);

    std::vector<std::string> files, syms;
    std::vector<Cat>         cats;
    discover(raw_dir, tf_suffix, flt, files, syms, cats);

    std::cout << "=== SPARTAK Portfolio Runner ===\n";
    std::cout << "Config         : " << config_path << "\n";
    std::cout << "Raw dir        : " << raw_dir << "\n";
    std::cout << "Timeframe      : " << tf << "\n";
    std::cout << "aggregate_bars : " << agg << "\n";
    std::cout << "compound       : " << (cfg.compound_sizing ? "yes" : "no") << "\n";
    std::cout << "spread_mult    : " << cfg.spread_mult << "\n";
    std::cout << "commission_mult: " << cfg.commission_mult << "\n";
    std::cout << "Universe       : " << files.size() << " symbols\n";
    for (std::size_t i = 0; i < syms.size(); ++i) {
        std::cout << "  [" << std::setw(6) << cat_name(cats[i]) << "] "
                  << syms[i] << "\n";
    }
    std::cout << "\n";

    if (list_only) return 0;
    if (files.empty()) { std::cerr << "Empty universe\n"; return 1; }

    cfg.verbose = true;

    try {
        engine::PortfolioBacktest bt(cfg);
        auto rep = bt.run(files);
        if (!rep.ok) {
            std::cerr << "Backtest failed: " << rep.error << "\n";
            return 1;
        }
        print_report(rep);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 2;
    }
}