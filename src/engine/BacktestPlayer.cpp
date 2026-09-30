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
// -----------------------------------------------------------------------------
// Хелпер: путь к соседнему .bin с другим ТФ.
// "EURUSD_M5.bin" + "_H1" -> "EURUSD_H1.bin"
// Если исходный уже "_H1.bin" — вернуть пустую строку.
// -----------------------------------------------------------------------------
static std::string sibling_tf_path(const std::string& file, const std::string& want_tf)
{
    if (file.empty()) return {};
    std::string p = file;
    for (auto& c : p) if (c == '\\') c = '/';
    const std::size_t slash = p.rfind('/');
    const std::string dir   = (slash == std::string::npos) ? "" : p.substr(0, slash + 1);
    const std::string name  = (slash == std::string::npos) ? p : p.substr(slash + 1);
    const std::size_t dot  = name.rfind('.');
    const std::string stem = (dot == std::string::npos) ? name : name.substr(0, dot);
    const std::string ext  = (dot == std::string::npos) ? "" : name.substr(dot);
    // Ищем последний "_XXX" в stem.
    const std::size_t us = stem.rfind('_');
    std::string base = stem;
    if (us != std::string::npos) {
        const std::string tail = stem.substr(us + 1);
        // Простые ТФ-суффиксы: M5, M15, M30, H1, H4, D1
        if (tail == "M1" || tail == "M5" || tail == "M15" || tail == "M30" ||
            tail == "H1" || tail == "H4" || tail == "D1" || tail == "W1") {
            base = stem.substr(0, us);
        }
    }
    return dir + base + "_" + want_tf + ext;
}
// -----------------------------------------------------------------------------
// Хелпер: агрегировать N баров в один.
// -----------------------------------------------------------------------------
static core::Bar aggregate_bucket(const std::vector<core::Bar>& bucket)
{
    core::Bar w{};
    w.timestamp   = bucket.front().timestamp;
    w.open        = bucket.front().open;
    w.close       = bucket.back().close;
    w.high        = bucket.front().high;
    w.low         = bucket.front().low;
    w.tick_volume = 0;
    int64_t spr_sum = 0;
    for (const auto& b : bucket) {
        if (b.high > w.high) w.high = b.high;
        if (b.low  < w.low)  w.low  = b.low;
        w.tick_volume += b.tick_volume;
        spr_sum += b.spread;
    }
    w.spread = static_cast<int32_t>(spr_sum / static_cast<int64_t>(bucket.size()));
    return w;
}
// -----------------------------------------------------------------------------
// run — главный цикл (multi-TF).
// -----------------------------------------------------------------------------
BacktestReport BacktestPlayer::run(const std::string& xfbar_path) {
    BacktestReport rep;
    // --- 1. Поток сигналов ---
    data::BarStream sig_stream(xfbar_path);
    if (!sig_stream.is_ok()) {
        rep.error = "Cannot open XFBAR file: " + xfbar_path;
        return rep;
    }
    if (const auto* rd = sig_stream.reader()) {
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
    rep.bars_total      = sig_stream.total_bars();
    rep.initial_balance = cfg_.initial_balance;
    rep.final_balance   = cfg_.initial_balance;
    rep.peak_balance    = cfg_.initial_balance;
    // --- 2. Открытие H1 и D1 потоков ---
    const std::string h1_path = cfg_.file_h1.empty() ? sibling_tf_path(xfbar_path, "H1") : cfg_.file_h1;
    const std::string d1_path = cfg_.file_d1.empty() ? sibling_tf_path(xfbar_path, "D1") : cfg_.file_d1;
    std::unique_ptr<data::BarStream> h1_stream;
    std::unique_ptr<data::BarStream> d1_stream;
    if (cfg_.use_multi_tf && !h1_path.empty()) {
        h1_stream = std::make_unique<data::BarStream>(h1_path);
        if (!h1_stream->is_ok()) { h1_stream.reset(); }
    }
    if (cfg_.use_multi_tf && !d1_path.empty()) {
        d1_stream = std::make_unique<data::BarStream>(d1_path);
        if (!d1_stream->is_ok()) { d1_stream.reset(); }
    }
    // --- 3. Skip irregular prefix ---
    if (cfg_.skip_irregular_prefix) {
        data::DataSanitizer sanitizer(cfg_.max_gap_ms, 10);
        auto srep = sanitizer.run(sig_stream);
        rep.bars_skipped_prefix = srep.bars_skipped;
        if (h1_stream) { data::DataSanitizer s2(cfg_.max_gap_ms, 10); s2.run(*h1_stream); }
        if (d1_stream) { data::DataSanitizer s3(cfg_.max_gap_ms, 10); s3.run(*d1_stream); }
    }
    // --- 4. Инициализация модулей ---
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
    engine::QuoteRateProvider rate_provider = engine::QuoteRateProvider::makeDefault();
    engine::PnlCalculator     pnl_calc(rate_provider);
    core::InstrumentSpec spec_local;
    if (!rep.symbol.empty()) {
        spec_local = core::InstrumentRegistry::resolve(rep.symbol);
    }
    position::PositionManager posman(cfg_.position, &pnl_calc, &spec_local);
    // --- 5. Главный цикл ---
    std::vector<core::Bar> sig_history;
    sig_history.reserve(cfg_.rolling_window + 10);
    std::vector<core::Bar> sig_bucket;
    sig_bucket.reserve(cfg_.aggregate_bars);
    std::vector<core::Bar> h1_history;
    h1_history.reserve(cfg_.rolling_window_h1 + 10);
    std::vector<core::Bar> h1_bucket;
    h1_bucket.reserve(cfg_.aggregate_bars_h1);
    std::vector<core::Bar> d1_history;
    d1_history.reserve(cfg_.rolling_window_d1 + 10);
    std::vector<core::Bar> d1_bucket;
    d1_bucket.reserve(cfg_.aggregate_bars_d1);
    core::Bar bar;
    std::size_t bar_index = 0;
    const std::size_t max_bars =
        (cfg_.max_bars > 0) ? cfg_.max_bars : static_cast<std::size_t>(1e12);
    while (bar_index < max_bars && sig_stream.next(bar)) {
        // 5.1. Агрегация сигнального ТФ
        sig_bucket.push_back(bar);
        if (sig_bucket.size() < cfg_.aggregate_bars) continue;
        const core::Bar wbar = aggregate_bucket(sig_bucket);
        sig_bucket.clear();
        sig_history.push_back(wbar);
        if (sig_history.size() > cfg_.rolling_window) {
            sig_history.erase(sig_history.begin(),
                              sig_history.begin() + (sig_history.size() - cfg_.rolling_window));
        }
        // 5.2. Агрегация H1 (поглощаем все бары H1 до wbar.timestamp)
        if (h1_stream) {
            core::Bar h1bar;
            while (h1_stream->next(h1bar)) {
                if (h1bar.timestamp > wbar.timestamp) {
                    h1_stream->push_back(h1bar);
                    break;
                }
                h1_bucket.push_back(h1bar);
                if (h1_bucket.size() >= cfg_.aggregate_bars_h1) {
                    h1_history.push_back(aggregate_bucket(h1_bucket));
                    h1_bucket.clear();
                    if (h1_history.size() > cfg_.rolling_window_h1) {
                        h1_history.erase(h1_history.begin(),
                                         h1_history.begin() + (h1_history.size() - cfg_.rolling_window_h1));
                    }
                }
            }
        }
        // 5.3. Агрегация D1
        if (d1_stream) {
            core::Bar d1bar;
            while (d1_stream->next(d1bar)) {
                if (d1bar.timestamp > wbar.timestamp) {
                    d1_stream->push_back(d1bar);
                    break;
                }
                d1_bucket.push_back(d1bar);
                if (d1_bucket.size() >= cfg_.aggregate_bars_d1) {
                    d1_history.push_back(aggregate_bucket(d1_bucket));
                    d1_bucket.clear();
                    if (d1_history.size() > cfg_.rolling_window_d1) {
                        d1_history.erase(d1_history.begin(),
                                         d1_history.begin() + (d1_history.size() - cfg_.rolling_window_d1));
                    }
                }
            }
        }
        // 5.4. Фильтр по диапазону дат
        if (cfg_.from_ms > 0 && wbar.timestamp < cfg_.from_ms) { ++bar_index; continue; }
        if (cfg_.to_ms   > 0 && wbar.timestamp > cfg_.to_ms)   { break; }
        // 5.5. Слишком мало истории сигналов
        if (sig_history.size() < cfg_.rolling_window) { ++bar_index; continue; }
        // 5.6. Анализ контекста
        core::MarketContext mctx;
        if (h1_stream && d1_stream &&
            h1_history.size() >= cfg_.rolling_window_h1 &&
            d1_history.size() >= cfg_.rolling_window_d1) {
            mctx = ctx_agg.analyze(d1_history, h1_history);
        } else if (h1_stream && h1_history.size() >= cfg_.rolling_window_h1) {
            mctx = ctx_agg.analyze({}, h1_history);
        } else {
            mctx = ctx_agg.analyze({}, sig_history);
        }
        // 5.7. Детект паттерна
        auto sig = pat_agg.analyze(sig_history, mctx);
        if (sig.detected) {
            ++rep.signals_detected;
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
        // 5.8. Обработка бара существующих позиций
        position::PositionAccountContext pac;
        pac.margin_level_pct = (account.margin_used > 0.0)
                             ? (account.equity / account.margin_used) * 100.0 : 1e9;
        pac.now_ms = wbar.timestamp;
        auto events = posman.onBar(wbar, pac);
        // 5.9. Применение событий
        for (const auto& e : events) {
            switch (e.type) {
                case position::PositionEventType::PartialClose:
                    ++rep.partial_closes;
                    account.balance += e.net_pnl;
                    rep.total_commission += e.commission;
                    rep.total_swap       += e.swap;
                    break;
                case position::PositionEventType::FullClose:
                    ++rep.full_closes;
                    account.balance += e.net_pnl;
                    rep.total_commission += e.commission;
                    rep.total_swap       += e.swap;
                    break;
                case position::PositionEventType::MoveBreakEven:
                    ++rep.be_moves; break;
                case position::PositionEventType::TrailingMove:
                    ++rep.trailing_moves; break;
                case position::PositionEventType::SwapAccrued:
                    ++rep.swaps_accrued; break;
                default: break;
            }
            if (rep.sample_events.size() < 20) {
                rep.sample_events.push_back(e);
            }
        }
        // 5.10. Equity / peak / drawdown
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
        // 5.11. Логирование
        if (cfg_.verbose && cfg_.log_every_n > 0 &&
            (bar_index % cfg_.log_every_n == 0)) {
            std::cout << "[BT] bar " << bar_index
                      << "  balance=" << std::fixed << std::setprecision(2)
                      << account.balance
                      << "  equity=" << account.equity
                      << "  active=" << posman.activeCount()
                      << "  h1=" << h1_history.size()
                      << "  d1=" << d1_history.size()
                      << "\n";
        }
        ++bar_index;
    }
    // --- 6. Финальный отчёт ---
    rep.bars_processed = bar_index;
    rep.net_pnl = rep.final_balance - rep.initial_balance;
    rep.return_pct = (rep.initial_balance > 0.0)
                   ? (rep.net_pnl / rep.initial_balance) * 100.0 : 0.0;
    rep.ok = true;
    return rep;
}} // namespace spartak::engine