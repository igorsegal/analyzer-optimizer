#include "engine/PortfolioBacktest.h"
#include "core/InstrumentRegistry.h"
#include "engine/QuoteRateProvider.h"
#include "engine/PnlCalculator.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <memory>
#include <stdexcept>
namespace spartak::engine {
namespace {
// -----------------------------------------------------------------------------
// Слот — состояние одного инструмента.
// -----------------------------------------------------------------------------
struct Slot {
    std::string             path;
    std::string             symbol;
    core::InstrumentSpec    spec;
    std::unique_ptr<data::BarStream> stream;   // открывается в load_slot
    bool                    eof       = false;
    bool                    has_bar   = false;
    core::Bar               current_bar{};
    std::vector<core::Bar>  history;
    std::unique_ptr<context::ContextAggregator>   ctx;
    std::unique_ptr<patterns::PatternAggregator>  pat;
    std::unique_ptr<validation::SignalValidator>  val;
    std::unique_ptr<position::PositionManager>    pm;
    std::size_t orders  = 0;
    double      net_pnl = 0.0;
    Slot() = default;
    Slot(const Slot&) = delete;
    Slot& operator=(const Slot&) = delete;
};
} // namespace
// -----------------------------------------------------------------------------
// Конструктор.
// -----------------------------------------------------------------------------
PortfolioBacktest::PortfolioBacktest(PortfolioConfig cfg) : cfg_(cfg) {
    if (cfg_.initial_balance <= 0.0)
        throw std::invalid_argument("initial_balance must be > 0");
    if (cfg_.risk_percent <= 0.0 || cfg_.risk_percent > 100.0)
        throw std::invalid_argument("risk_percent must be in (0,100]");
    if (cfg_.min_margin_level_pct < 0.0)
        throw std::invalid_argument("min_margin_level_pct must be >= 0");
    // Risk percent должен быть прокинут во все вложенные конфиги
    cfg_.validation.risk_percent = cfg_.risk_percent;
    cfg_.validation.money.risk_percent = cfg_.risk_percent;
}
// -----------------------------------------------------------------------------
// Загрузка одного инструмента.
// -----------------------------------------------------------------------------
static std::unique_ptr<Slot> load_slot(const std::string& path,
                                       const PortfolioConfig& cfg,
                                       const QuoteRateProvider& rates)
{
    auto slot = std::make_unique<Slot>();
    slot->path = path;
    slot->stream = std::make_unique<data::BarStream>(path, 4096);
    if (!slot->stream->is_ok()) {
        std::cerr << "[Portfolio] Cannot open: " << path << "\n";
        return nullptr;
    }
    // Symbol и spec
    if (const auto* rd = slot->stream->reader()) {
        slot->symbol = rd->symbol();
    }
    slot->spec = core::InstrumentRegistry::resolve(slot->symbol);
    // Локальные конфиги для этого инструмента
    auto ctx_cfg  = cfg.context;
    auto pat_cfg  = cfg.pattern;
    auto val_cfg  = cfg.validation;
    auto pm_cfg   = cfg.position;
    ctx_cfg.point = slot->spec.point;
    pat_cfg.point = slot->spec.point;
    pat_cfg.pinbar_buy.stop_buffer_points  = slot->spec.stop_buffer_points;
    pat_cfg.pinbar_sell.stop_buffer_points = slot->spec.stop_buffer_points;
    pat_cfg.engulfing.stop_buffer_points   = slot->spec.stop_buffer_points;
    pat_cfg.impulse.stop_buffer_points     = slot->spec.stop_buffer_points;
    val_cfg.point                = slot->spec.point;
    val_cfg.contract_size        = slot->spec.contract_size;
    val_cfg.lot_rounder.min_lot  = slot->spec.min_lot;
    val_cfg.lot_rounder.max_lot  = slot->spec.max_lot;
    val_cfg.lot_rounder.lot_step = slot->spec.lot_step;
    val_cfg.money.contract_size  = slot->spec.contract_size;
    val_cfg.money.point          = slot->spec.point;
    val_cfg.margin.contract_size = slot->spec.contract_size;
    pm_cfg.point                    = slot->spec.point;
    pm_cfg.contract_size            = slot->spec.contract_size;
    pm_cfg.splitter.min_lot         = slot->spec.min_lot;
    pm_cfg.splitter.lot_step        = slot->spec.lot_step;
    pm_cfg.breakeven.point          = slot->spec.point;
    pm_cfg.swap.point               = slot->spec.point;
    pm_cfg.swap.contract_size       = slot->spec.contract_size;
    pm_cfg.swap.swap_long_points    = slot->spec.swap_long_points;
    pm_cfg.swap.swap_short_points   = slot->spec.swap_short_points;
    pm_cfg.commission.commission_per_lot = slot->spec.commission_per_lot;
    pm_cfg.trailing.point           = slot->spec.point;
    pm_cfg.trailing.trailing_distance_points = slot->spec.trailing_distance_points;
    pm_cfg.use_trailing             = true;
    pm_cfg.trail_only_after_tp1     = true;
    pm_cfg.emergency_enabled        = false;   // отключено, риск-контроль через margin
    // Modules
    slot->ctx = std::make_unique<context::ContextAggregator>(ctx_cfg);
    slot->pat = std::make_unique<patterns::PatternAggregator>(pat_cfg);
    slot->val = std::make_unique<validation::SignalValidator>(val_cfg);
    // PnL-калькулятор общий, но передаём указатель — spec живёт в slot.
    // Храним указатель на spec внутри PositionManager.
    static engine::PnlCalculator pnl_calc_static(rates);   // один экземпляр на всю портфельную сессию
    slot->pm = std::make_unique<position::PositionManager>(
        pm_cfg, &pnl_calc_static, &slot->spec);
    return slot;
}
// -----------------------------------------------------------------------------
// run — главный цикл.
// -----------------------------------------------------------------------------
PortfolioReport PortfolioBacktest::run(const std::vector<std::string>& files) {
    PortfolioReport rep;
    rep.initial_balance = cfg_.initial_balance;
    rep.final_balance   = cfg_.initial_balance;
    rep.peak_equity     = cfg_.initial_balance;
    if (files.empty()) {
        rep.error = "no files provided";
        return rep;
    }
    // --- 1. Инициализация ---
    engine::QuoteRateProvider rates = engine::QuoteRateProvider::makeDefault();
    std::vector<std::unique_ptr<Slot>> slots;
    slots.reserve(files.size());
    for (const auto& f : files) {
        auto s = load_slot(f, cfg_, rates);
        if (!s) { ++rep.instruments_failed; continue; }
        // Читаем первый бар
        if (s->stream->next(s->current_bar)) {
            s->has_bar = true;
        } else {
            s->eof = true;
        }
        slots.push_back(std::move(s));
        ++rep.instruments_loaded;
    }
    if (slots.empty()) {
        rep.error = "no instruments loaded";
        return rep;
    }
    std::cout << "[Portfolio] Loaded " << rep.instruments_loaded
              << " instruments, failed " << rep.instruments_failed << "\n";
    std::cout << "[Portfolio] Risk per trade: " << cfg_.risk_percent
              << "%, min margin level: " << cfg_.min_margin_level_pct << "%\n\n";
    // --- 2. Общий счёт ---
    validation::AccountSnapshot account;
    account.balance              = cfg_.initial_balance;
    account.equity               = cfg_.initial_balance;
    account.free_margin          = cfg_.initial_balance;
    account.margin_used          = 0.0;
    account.leverage             = 500.0;   // для золота/индексов может отличаться, но усредним
    account.min_margin_level_pct = cfg_.min_margin_level_pct;
    // --- 3. Главный синхронный цикл по timestamp ---
    bool any_bar = true;
    while (any_bar) {
        // Ищем минимальный timestamp среди непустых слотов
        int64_t min_ts = INT64_MAX;
        for (auto& s : slots) {
            if (s->has_bar && s->current_bar.timestamp < min_ts)
                min_ts = s->current_bar.timestamp;
        }
        if (min_ts == INT64_MAX) break;
        // Сначала обновим equity/free_margin от всех открытых позиций
        double margin_used_total = 0.0;
        double unrealized_total  = 0.0;
        for (auto& s : slots) {
            for (const auto& p : s->pm->positions()) {
                if (!p.is_active()) continue;
                const double close_px = (p.side == core::OrderSide::Buy)
                                      ? s->current_bar.close
                                      : s->current_bar.close +
                                        s->current_bar.spread * s->spec.point;
                const double dir = (p.side == core::OrderSide::Buy) ? 1.0 : -1.0;
                unrealized_total += (close_px - p.entry_price) * dir
                                  * p.remaining_volume * s->spec.contract_size;
                margin_used_total += (p.remaining_volume * s->spec.contract_size)
                                   / s->spec.leverage;
            }
        }
        account.margin_used = margin_used_total;
        account.equity      = account.balance + unrealized_total;
        account.free_margin = account.equity - margin_used_total;
        if (account.equity > rep.peak_equity) rep.peak_equity = account.equity;
        if (rep.peak_equity > 0.0) {
            const double dd = (rep.peak_equity - account.equity)
                            / rep.peak_equity * 100.0;
            if (dd > rep.max_drawdown_pct) rep.max_drawdown_pct = dd;
        }
        // Обрабатываем каждый слот с минимальным timestamp
        for (auto& s : slots) {
            if (!s->has_bar || s->current_bar.timestamp != min_ts) continue;
            const core::Bar bar = s->current_bar;
            // История
            s->history.push_back(bar);
            if (s->history.size() > cfg_.context.max_zones_per_tf * 100)
                s->history.erase(s->history.begin(),
                                 s->history.begin() + (s->history.size() -
                                    cfg_.context.max_zones_per_tf * 100));
            ++rep.bars_processed;
            // 4.1. Анализ контекста
            auto mctx = s->ctx->analyze({}, s->history);
            // 4.2. Детект паттерна
            auto sig = s->pat->analyze(s->history, mctx);
            if (sig.detected) {
                ++rep.signals_total;
                // 4.3. Валидация (передаём общий snapshot)
                auto vr = s->val->validate(sig, mctx, bar, account);
                if (vr.is_approved) {
                    ++rep.orders_approved;
                    s->pm->openPosition(vr.order, bar.spread, bar.timestamp);
                    ++s->orders;
                } else {
                    ++rep.orders_rejected;
                    if (vr.reason == core::RejectReason::MarginCall)
                        ++rep.orders_margin_blocked;
                }
            }
            // 4.4. Обработка позиций на баре
            position::PositionAccountContext pac;
            pac.margin_level_pct = (account.margin_used > 0.0)
                                 ? (account.equity / account.margin_used) * 100.0
                                 : 1e9;
            pac.now_ms = bar.timestamp;
            auto events = s->pm->onBar(bar, pac);
            for (const auto& e : events) {
                if (e.type == position::PositionEventType::PartialClose ||
                    e.type == position::PositionEventType::FullClose)
                {
                    account.balance += e.net_pnl;
                    s->net_pnl      += e.net_pnl;
                }
            }
            // 4.5. Читаем следующий бар
            if (!s->stream->next(s->current_bar)) {
                s->eof     = true;
                s->has_bar = false;
            }
        }
        if (rep.first_time_ms == 0) rep.first_time_ms = min_ts;
        rep.last_time_ms = min_ts;
    }
    // --- 4. Финальный отчёт ---
    rep.final_balance = account.balance;
    rep.net_pnl       = rep.final_balance - rep.initial_balance;
    rep.return_pct    = (rep.initial_balance > 0.0)
                      ? (rep.net_pnl / rep.initial_balance) * 100.0
                      : 0.0;
    rep.per_instrument.reserve(slots.size());
    for (auto& s : slots) {
        rep.per_instrument.push_back({ s->symbol, s->orders, s->net_pnl });
    }
    std::sort(rep.per_instrument.begin(), rep.per_instrument.end(),
              [](const auto& a, const auto& b) { return a.net_pnl > b.net_pnl; });
    rep.ok = true;
    return rep;
}
} // namespace spartak::engine