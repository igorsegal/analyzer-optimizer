#include "engine/BacktestPlayer.h"
#include "core/InstrumentRegistry.h"
#include "engine/PnlCalculator.h"
#include "engine/QuoteRateProvider.h"
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
    // --- Автоконфигурация по InstrumentRegistry ---
    if (!rep.symbol.empty()) {
        auto spec = core::InstrumentRegistry::resolve(rep.symbol);
        cfg_.point              = spec.point;
        cfg_.contract_size      = spec.contract_size;
        cfg_.leverage           = spec.leverage;
        cfg_.commission_per_lot = spec.commission_per_lot;
        cfg_.context.point = spec.point;
        cfg_.pattern.point = spec.point;
        cfg_.validation.point                 = spec.point;
        cfg_.validation.contract_size         = spec.contract_size;
        cfg_.validation.lot_rounder.min_lot   = spec.min_lot;
        cfg_.validation.lot_rounder.max_lot   = spec.max_lot;
        cfg_.validation.lot_rounder.lot_step  = spec.lot_step;
        cfg_.validation.money.contract_size   = spec.contract_size;
        cfg_.validation.money.point           = spec.point;
        cfg_.validation.margin.contract_size  = spec.contract_size;
        // --- Per-instrument distances (stop buffer + trailing) ---
        cfg_.pattern.pinbar_buy.stop_buffer_points   = spec.stop_buffer_points;
        cfg_.pattern.pinbar_sell.stop_buffer_points  = spec.stop_buffer_points;
        cfg_.pattern.engulfing.stop_buffer_points    = spec.stop_buffer_points;
        cfg_.pattern.impulse.stop_buffer_points      = spec.stop_buffer_points;
        cfg_.position.trailing.trailing_distance_points = spec.trailing_distance_points;
        cfg_.position.point                 = spec.point;
        cfg_.position.contract_size         = spec.contract_size;
        cfg_.position.splitter.min_lot      = spec.min_lot;
        cfg_.position.splitter.lot_step     = spec.lot_step;
        cfg_.position.breakeven.point       = spec.point;
        cfg_.position.swap.point            = spec.point;
        cfg_.position.swap.contract_size    = spec.contract_size;
        cfg_.position.swap.swap_long_points  = spec.swap_long_points;
        cfg_.position.swap.swap_short_points = spec.swap_short_points;
        cfg_.position.commission.commission_per_lot = spec.commission_per_lot;
        cfg_.position.trailing.point        = spec.point;
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
    // PnL калькулятор с курсами валют (единый источник конверсии в USD)
    engine::QuoteRateProvider rate_provider = engine::QuoteRateProvider::makeDefault();
    engine::PnlCalculator     pnl_calc(rate_provider);
    // Сохраняем spec в локальную переменную, чтобы передать в PositionManager
    core::InstrumentSpec spec_local;
    if (!rep.symbol.empty()) {
        spec_local = core::InstrumentRegistry::resolve(rep.symbol);
    }
    
    position::PositionManager posman(cfg_.position, &pnl_calc, &spec_local);
    // --- 4. Главный цикл ---
    std::vector<core::Bar> history;
    history.reserve(cfg_.rolling_window + 10);
    std::vector<core::Bar> agg_bucket;
    agg_bucket.reserve(cfg_.aggregate_bars);

    core::Bar bar;
    std::size_t bar_index = 0;
    const std::size_t max_bars =
        (cfg_.max_bars > 0) ? cfg_.max_bars : static_cast<std::size_t>(1e12);
    while (bar_index < max_bars && stream.next(bar)) {
        // --- Агрегация N исходных баров в один рабочий ---
        agg_bucket.push_back(bar);
        if (agg_bucket.size() < cfg_.aggregate_bars) continue;
        core::Bar wbar{};
        wbar.timestamp   = agg_bucket.front().timestamp;
        wbar.open        = agg_bucket.front().open;
        wbar.close       = agg_bucket.back().close;
        wbar.high        = agg_bucket.front().high;
        wbar.low         = agg_bucket.front().low;
        wbar.tick_volume = 0;
        int64_t spr_sum = 0;
        for (const auto& b : agg_bucket) {
            if (b.high > wbar.high) wbar.high = b.high;
            if (b.low  < wbar.low)  wbar.low  = b.low;
            wbar.tick_volume += b.tick_volume;
            spr_sum += b.spread;
        }
        wbar.spread = static_cast<int32_t>(spr_sum / static_cast<int64_t>(agg_bucket.size()));
        agg_bucket.clear();
        history.push_back(wbar);
        // Rolling window
        if (history.size() > cfg_.rolling_window) {
            history.erase(history.begin(),
                          history.begin() + (history.size() - cfg_.rolling_window));
        }
        // Фильтр по диапазону дат
        if (cfg_.from_ms > 0 && wbar.timestamp < cfg_.from_ms) {
            ++bar_index;
            continue;
        }
        if (cfg_.to_ms > 0 && wbar.timestamp > cfg_.to_ms) {
            break;
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
                auto vr = validator.validate(sig, mctx, wbar, account);
                if (vr.is_approved) {
                    ++rep.orders_approved;
                    posman.openPosition(vr.order, wbar.spread, wbar.timestamp);
                } else {
                    ++rep.orders_rejected;
                    switch (vr.reason) {
                        case core::RejectReason::SpreadTooHigh: ++rep.reject_spread;  break;
                        case core::RejectReason::TrendConflict: ++rep.reject_trend;   break;
                        case core::RejectReason::MarginCall:
                        case core::RejectReason::MarginTooLow:  ++rep.reject_margin;  break;
                        case core::RejectReason::SessionClosed: ++rep.reject_session; break;
                        default: ++rep.reject_other; break;
                    }
                }
            }
        }
        // 4.4. Обработка бара существующих позиций
        position::PositionAccountContext pac;
        pac.margin_level_pct = (account.margin_used > 0.0)
                             ? (account.equity / account.margin_used) * 100.0
                             : 1e9;
        pac.now_ms = wbar.timestamp;
        auto events = posman.onBar(wbar, pac);
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
            live_pnl += (wbar.close - s.entry_price) * dir
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