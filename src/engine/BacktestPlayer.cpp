#include "engine/BacktestPlayer.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <stdexcept>
namespace spartak::engine {
// -----------------------------------------------------------------------------
// Конструктор.
// -----------------------------------------------------------------------------
BacktestPlayer::BacktestPlayer(BacktestConfig cfg)
    : cfg_(cfg) {
    if (cfg_.point <= 0.0)
        throw std::invalid_argument("BacktestConfig::point must be > 0");
    if (cfg_.initial_balance <= 0.0)
        throw std::invalid_argument("BacktestConfig::initial_balance must be > 0");
    if (cfg_.rolling_window < 50)
        throw std::invalid_argument("BacktestConfig::rolling_window must be >= 50");
    if (cfg_.contract_size <= 0.0)
        throw std::invalid_argument("BacktestConfig::contract_size must be > 0");
}
// -----------------------------------------------------------------------------
// run — главный цикл.
// -----------------------------------------------------------------------------
BacktestReport BacktestPlayer::run(const std::string& xfbar_path) {
    BacktestReport rep;
    // --- 1. Открытие потока ---
    data::BarStream stream(xfbar_path);
    if (!stream.is_ok()) {
        rep.error = "Cannot open XFBAR file: " + xfbar_path;
        return rep;
    }
    if (const auto* rd = stream.reader()) {
        rep.symbol         = rd->symbol();
        rep.period_seconds = rd->header().period_seconds;
    }
    rep.bars_total       = stream.total_bars();
    rep.initial_balance  = cfg_.initial_balance;
    rep.final_balance    = cfg_.initial_balance;
    rep.peak_balance     = cfg_.initial_balance;
    // --- 2. Skip irregular prefix ---
    if (cfg_.skip_irregular_prefix) {
        data::DataSanitizer sanitizer(cfg_.max_gap_ms, 10);
        auto srep = sanitizer.run(stream);
        rep.bars_skipped_prefix = srep.bars_skipped;
    }
    // --- 3. Инициализация модулей ---
    context::ContextAggregator  ctx_agg(cfg_.context);
    patterns::PatternAggregator pat_agg(cfg_.pattern);
    validation::AccountSnapshot account;
    account.balance              = cfg_.initial_balance;
    account.equity               = cfg_.initial_balance;
    account.free_margin          = cfg_.initial_balance;
    account.margin_used          = 0.0;
    account.leverage             = cfg_.leverage;
    account.commission_per_lot   = cfg_.commission_per_lot;
    account.min_margin_level_pct = cfg_.min_margin_level_pct;
    validation::SignalValidator validator(cfg_.validation);
    position::PositionManager   posman(cfg_.position);
    // --- 4. Главный цикл ---
    std::vector<core::Bar> history;
    history.reserve(cfg_.rolling_window + 10);
    core::Bar bar;
    std::size_t bar_index = 0;
    const std::size_t max_bars =
        (cfg_.max_bars > 0) ? cfg_.max_bars : static_cast<std::size_t>(1e12);
    while (bar_index < max_bars && stream.next(bar)) {
        history.push_back(bar);
        // Rolling window: не даём вектору расти бесконечно
        if (history.size() > cfg_.rolling_window) {
            history.erase(history.begin(),
                          history.begin() + (history.size() - cfg_.rolling_window));
        }
        // Слишком мало истории — пропускаем
        if (history.size() < cfg_.rolling_window) {
            ++bar_index;
            continue;
        }
        // 4.1. Анализ контекста
        auto mctx = ctx_agg.analyze({}, history);
        // 4.2. Детект паттерна
        auto sig = pat_agg.analyze(history, mctx);
        if (sig.detected) {
            ++rep.signals_detected;
            // 4.3. Валидация (только если нет активной позиции)
            const bool can_open = (posman.activeCount() == 0);
            if (!can_open) {
                ++rep.orders_rejected;
            } else {
                auto vr = validator.validate(sig, mctx, bar, account);
                if (vr.is_approved) {
                    ++rep.orders_approved;
                    posman.openPosition(vr.order, bar.spread, bar.timestamp);
                } else {
                    ++rep.orders_rejected;
                }
            }
        }
        // 4.4. Обработка бара существующих позиций
        position::PositionAccountContext pac;
        pac.margin_level_pct = (account.margin_used > 0.0)
                             ? (account.equity / account.margin_used) * 100.0
                             : 1e9;
        pac.now_ms = bar.timestamp;
        auto events = posman.onBar(bar, pac);
        // 4.5. Обрабатываем события: применяем к балансу
        for (const auto& e : events) {
            switch (e.type) {
                case position::PositionEventType::PartialClose: {
                    ++rep.partial_closes;
                    account.balance += e.net_pnl;
                    rep.total_commission += e.commission;
                    rep.total_swap       += e.swap;
                    break;
                }
                case position::PositionEventType::FullClose: {
                    ++rep.full_closes;
                    account.balance += e.net_pnl;
                    rep.total_commission += e.commission;
                    rep.total_swap       += e.swap;
                    break;
                }
                case position::PositionEventType::MoveBreakEven:
                    ++rep.be_moves;
                    break;
                case position::PositionEventType::TrailingMove:
                    ++rep.trailing_moves;
                    break;
                case position::PositionEventType::SwapAccrued:
                    ++rep.swaps_accrued;
                    break;
                default:
                    break;
            }
            if (rep.sample_events.size() < 20) {
                rep.sample_events.push_back(e);
            }
        }
        // 4.6. Пересчёт equity / peak / drawdown
        double live_pnl = 0.0;
        double margin_used = 0.0;
        for (const auto& s : posman.positions()) {
            if (!s.is_active()) continue;
            const double dir = (s.side == core::OrderSide::Buy) ? 1.0 : -1.0;
            live_pnl += (bar.close - s.entry_price) * dir
                      * s.remaining_volume * cfg_.contract_size;
            margin_used += (s.remaining_volume * cfg_.contract_size) / cfg_.leverage;
        }
        account.equity      = account.balance + live_pnl;
        account.margin_used = margin_used;
        account.free_margin = account.equity - margin_used;
        rep.final_balance = account.balance;
        if (account.equity > rep.peak_balance) rep.peak_balance = account.equity;
        if (rep.peak_balance > 0.0) {
            const double dd = (rep.peak_balance - account.equity)
                            / rep.peak_balance * 100.0;
            if (dd > rep.max_drawdown_pct) rep.max_drawdown_pct = dd;
        }
        // 4.7. Логирование прогресса
        if (cfg_.verbose && cfg_.log_every_n > 0 &&
            (bar_index % cfg_.log_every_n == 0))
        {
            std::cout << "[BT] bar " << bar_index
                      << "  balance=" << std::fixed << std::setprecision(2)
                      << account.balance
                      << "  equity=" << account.equity
                      << "  active=" << posman.activeCount()
                      << "\n";
        }
        ++bar_index;
    }
    // --- 5. Финальный отчёт ---
    rep.bars_processed = bar_index;
    rep.net_pnl = rep.final_balance - rep.initial_balance;
    rep.return_pct = (rep.initial_balance > 0.0)
                   ? (rep.net_pnl / rep.initial_balance) * 100.0
                   : 0.0;
    rep.ok = true;
    return rep;
}
} // namespace spartak::engine