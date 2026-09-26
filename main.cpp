// =============================================================================
//  SPARTAK :: main.cpp
//  Точка входа бэктестера.
//
//  Запуск:
//    spartak.exe --file <path_to_xfbar>
//    spartak.exe --file <path> --balance 20000
//    spartak.exe --file <path> --no-skip-prefix
//    spartak.exe --help
// =============================================================================
#include "engine/BacktestPlayer.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
using namespace spartak;
static void print_usage() {
    std::cout <<
        "SPARTAK Backtest Player\n"
        "Usage:\n"
        "  spartak.exe --file <path_to_xfbar> [options]\n"
        "\n"
        "Options:\n"
        "  --file <path>        Path to XFBAR .bin file (required)\n"
        "  --balance <amount>   Initial balance (default: 10000)\n"
        "  --window <N>         Rolling window bars (default: 100)\n"
        "  --no-skip-prefix     Do not skip irregular data prefix\n"
        "  --max-bars <N>       Process only first N bars (0 = all)\n"
        "  --quiet              Suppress progress logging\n"
        "  --log-every <N>      Print progress every N bars\n"
        "  --help               Show this message\n";
}
static void print_report(const engine::BacktestReport& r) {
    std::cout << "\n";
    std::cout << "===============================================================\n";
    std::cout << " SPARTAK BACKTEST REPORT\n";
    std::cout << "===============================================================\n";
    std::cout << " Symbol              : " << r.symbol << "\n";
    std::cout << " Period (sec)        : " << r.period_seconds << "\n";
    std::cout << " Bars total          : " << r.bars_total << "\n";
    std::cout << " Bars skipped prefix : " << r.bars_skipped_prefix << "\n";
    std::cout << " Bars processed      : " << r.bars_processed << "\n";
    std::cout << "\n";
    std::cout << " Signals detected    : " << r.signals_detected << "\n";
    std::cout << " Orders approved     : " << r.orders_approved << "\n";
    std::cout << " Orders rejected     : " << r.orders_rejected << "\n";
    std::cout << "\n";
    std::cout << " Partial closes      : " << r.partial_closes << "\n";
    std::cout << " Full closes         : " << r.full_closes << "\n";
    std::cout << " Break-even moves    : " << r.be_moves << "\n";
    std::cout << " Trailing moves      : " << r.trailing_moves << "\n";
    std::cout << " Swaps accrued       : " << r.swaps_accrued << "\n";
    std::cout << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << " Initial balance     : $" << r.initial_balance << "\n";
    std::cout << " Final balance       : $" << r.final_balance << "\n";
    std::cout << " Net PnL             : $" << r.net_pnl << "\n";
    std::cout << " Return              : " << r.return_pct << " %\n";
    std::cout << " Peak balance        : $" << r.peak_balance << "\n";
    std::cout << " Max drawdown        : " << r.max_drawdown_pct << " %\n";
    std::cout << " Total commission    : $" << r.total_commission << "\n";
    std::cout << " Total swap          : $" << r.total_swap << "\n";
    std::cout << "===============================================================\n";
    if (!r.sample_events.empty()) {
        std::cout << "\nSample events (first " << r.sample_events.size() << "):\n";
        std::cout << std::setprecision(5);
        for (const auto& e : r.sample_events) {
            std::cout << "  " << position::to_string(e.type)
                      << "  id=" << e.position_id
                      << "  price=" << e.price;
            if (e.volume > 0) std::cout << "  vol=" << std::setprecision(2) << e.volume;
            if (e.new_sl > 0) std::cout << "  sl=" << std::setprecision(5) << e.new_sl;
            if (e.net_pnl != 0.0) std::cout << "  net=$" << std::setprecision(2) << e.net_pnl;
            if (!e.reason.empty()) std::cout << "  [" << e.reason << "]";
            std::cout << "\n";
            std::cout << std::setprecision(5);
        }
    }
}
int main(int argc, char** argv) {
    engine::BacktestConfig cfg;
    std::string path;
    bool quiet = false;
    std::size_t log_every = 0;
    // --- Парсинг аргументов ---
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--file" && i + 1 < argc) {
            path = argv[++i];
        } else if (a == "--balance" && i + 1 < argc) {
            cfg.initial_balance = std::stod(argv[++i]);
        } else if (a == "--window" && i + 1 < argc) {
            cfg.rolling_window = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (a == "--no-skip-prefix") {
            cfg.skip_irregular_prefix = false;
        } else if (a == "--max-bars" && i + 1 < argc) {
            cfg.max_bars = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (a == "--quiet") {
            quiet = true;
        } else if (a == "--log-every" && i + 1 < argc) {
            log_every = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (a == "--help" || a == "-h") {
            print_usage();
            return 0;
        } else {
            std::cerr << "Unknown argument: " << a << "\n";
            print_usage();
            return 1;
        }
    }
    if (path.empty()) {
        std::cerr << "Error: --file is required\n\n";
        print_usage();
        return 1;
    }
    cfg.verbose   = !quiet;
    cfg.log_every_n = log_every;
    // --- Разумные дефолты стратегии для бэктеста ---
    // Контекст: тестируем на одном ТФ
    cfg.context.use_same_tf_for_both = true;
    // Паттерны: жёсткая фильтрация (только значимые бары + трендовое согласие)
    cfg.pattern.volume.min_volume = 50;   // бар должен иметь >= 50 тиков
    cfg.pattern.volume.avg_ratio  = 0.5;  // и >= 50% от среднего
    cfg.pattern.require_trend_for_consolidation = true;
    // Валидация
    cfg.validation.lot_rounder.max_lot = 1.0;       // жёсткий потолок
    cfg.validation.session.enabled     = false;     // любое время суток
    // Позиция: трейлинг только после TP1, emergency отключён на первом прогоне
    cfg.position.use_trailing       = true;
    cfg.position.trail_only_after_tp1 = true;
    cfg.position.emergency_enabled  = false;
    cfg.position.commission.commission_per_lot = 3.0;
    // Trailing: расстояние 50 пунктов, активация после 100
    cfg.position.trailing.trailing_distance_points = 50;
    cfg.position.trailing.activation_points        = 100;
    std::cout << "=== SPARTAK Backtest Player ===\n";
    std::cout << "File    : " << path << "\n";
    std::cout << "Balance : $" << std::fixed << std::setprecision(2)
              << cfg.initial_balance << "\n";
    std::cout << "Window  : " << cfg.rolling_window << " bars\n\n";
    // --- Прогон ---
    try {
        engine::BacktestPlayer player(cfg);
        auto report = player.run(path);
        if (!report.ok) {
            std::cerr << "Backtest failed: " << report.error << "\n";
            return 1;
        }
        print_report(report);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 2;
    }
}