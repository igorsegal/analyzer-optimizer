// =============================================================================
//  SPARTAK :: engine/PortfolioBacktest.cpp
// =============================================================================
#include "engine/PortfolioBacktest.h"
#include "core/InstrumentRegistry.h"
#include "engine/QuoteRateProvider.h"
#include "engine/PnlCalculator.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <map>
#include <cstdint>
#include <ctime>
#include <cstdio>

namespace spartak::engine {
namespace {

struct Slot {
    std::string             path;
    std::string             symbol;
    core::InstrumentSpec    spec;
    std::unique_ptr<data::BarStream> stream;
    bool                    eof       = false;
    bool                    has_bar   = false;
    core::Bar               current_bar{};

    std::vector<core::Bar>  sig_history;
    std::vector<core::Bar>  ctx_history;
    core::Bar               ctx_accum{};
    std::size_t             ctx_count = 0;
    bool                    ctx_accum_active = false;

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

struct TradeMeta {
    double       accumulated_pnl = 0.0;
    int          pattern_type    = 0;
    int          pattern_side    = 0;
};

bool skip_signal(const core::PatternSignal& sig, const PortfolioConfig& cfg) {
    const bool is_buy  = (sig.side == core::OrderSide::Buy);
    const bool is_sell = !is_buy;
    using PT = core::PatternType;
    if (cfg.skip_false_breakout && sig.type == PT::FalseBreakout) return true;
    if (cfg.skip_impulse_buy   && sig.type == PT::ImpulseBreakout && is_buy)  return true;
    if (cfg.skip_impulse_sell  && sig.type == PT::ImpulseBreakout && is_sell) return true;
    if (cfg.skip_consolidation_buy  && sig.type == PT::Consolidation && is_buy)  return true;
    if (cfg.skip_consolidation_sell && sig.type == PT::Consolidation && is_sell) return true;
    return false;
}

void to_utc(int64_t ms, std::tm& out) {
    std::time_t secs = static_cast<std::time_t>(ms / 1000);
#if defined(_MSC_VER)
    gmtime_s(&out, &secs);
#else
    gmtime_r(&secs, &out);
#endif
}

int year_from_ms(int64_t ms) {
    std::tm tm{};
    to_utc(ms, tm);
    return tm.tm_year + 1900;
}

int day_key_from_ms(int64_t ms) { return static_cast<int>(ms / (86400LL * 1000LL)); }
int month_key_from_ms(int64_t ms) {
    std::tm tm{};
    to_utc(ms, tm);
    return (tm.tm_year + 1900) * 12 + tm.tm_mon;
}

std::string day_str_from_ms(int64_t ms) {
    std::tm tm{};
    to_utc(ms, tm);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return buf;
}

void ctx_accum_push(core::Bar& accum, bool& active,
                    std::size_t& count, const core::Bar& b)
{
    if (!active) {
        accum = b;
        accum.spread = b.spread;
        active = true;
        count = 1;
    } else {
        accum.high = std::max(accum.high, b.high);
        accum.low  = std::min(accum.low,  b.low);
        accum.close = b.close;
        accum.tick_volume += b.tick_volume;
        accum.spread = b.spread;
        ++count;
    }
}

} // namespace

PortfolioBacktest::PortfolioBacktest(PortfolioConfig cfg) : cfg_(cfg) {
    if (cfg_.initial_balance <= 0.0)
        throw std::invalid_argument("initial_balance must be > 0");
    if (cfg_.risk_percent <= 0.0 || cfg_.risk_percent > 100.0)
        throw std::invalid_argument("risk_percent must be in (0,100]");
    if (cfg_.min_margin_level_pct < 0.0)
        throw std::invalid_argument("min_margin_level_pct must be >= 0");
    if (cfg_.min_rr < 0.0)
        throw std::invalid_argument("min_rr must be >= 0");
    if (cfg_.aggregate_bars == 0)
        throw std::invalid_argument("aggregate_bars must be >= 1");
    if (cfg_.spread_mult <= 0.0)
        throw std::invalid_argument("spread_mult must be > 0");
    if (cfg_.commission_mult <= 0.0)
        throw std::invalid_argument("commission_mult must be > 0");
    cfg_.validation.risk_percent       = cfg_.risk_percent;
    cfg_.validation.money.risk_percent = cfg_.risk_percent;
}

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
    if (const auto* rd = slot->stream->reader()) {
        slot->symbol = rd->symbol();
    }
    slot->spec = core::InstrumentRegistry::resolve(slot->symbol);

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

    // --- commission_mult ---
    pm_cfg.commission.commission_per_lot = slot->spec.commission_per_lot
                                         * cfg.commission_mult;

    pm_cfg.trailing.point           = slot->spec.point;
    pm_cfg.trailing.trailing_distance_points = slot->spec.trailing_distance_points;
    pm_cfg.use_trailing             = true;
    pm_cfg.trail_only_after_tp1     = true;
    pm_cfg.emergency_enabled        = false;

    slot->ctx = std::make_unique<context::ContextAggregator>(ctx_cfg);
    slot->pat = std::make_unique<patterns::PatternAggregator>(pat_cfg);
    slot->val = std::make_unique<validation::SignalValidator>(val_cfg);

    static engine::PnlCalculator pnl_calc_static(rates);
    slot->pm = std::make_unique<position::PositionManager>(
        pm_cfg, &pnl_calc_static, &slot->spec);
    return slot;
}

PortfolioReport PortfolioBacktest::run(const std::vector<std::string>& files) {
    PortfolioReport rep;
    rep.initial_balance = cfg_.initial_balance;
    rep.final_balance   = cfg_.initial_balance;
    rep.peak_equity     = cfg_.initial_balance;

    if (run_called_)
        std::cerr << "[Portfolio] warning: run() called twice\n";
    run_called_ = true;

    if (files.empty()) {
        rep.error = "no files provided";
        return rep;
    }

    engine::QuoteRateProvider rates = engine::QuoteRateProvider::makeDefault();
    std::vector<std::unique_ptr<Slot>> slots;
    slots.reserve(files.size());
    for (const auto& f : files) {
        auto s = load_slot(f, cfg_, rates);
        if (!s) { ++rep.instruments_failed; continue; }

        bool ok = true;
        while (ok && s->stream->next(s->current_bar)) {
            if (cfg_.from_ms > 0 && s->current_bar.timestamp < cfg_.from_ms)
                continue;
            break;
        }
        if (s->current_bar.timestamp >= (cfg_.from_ms > 0 ? cfg_.from_ms : 0)
            && !s->eof) {
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
              << "%, min margin level: " << cfg_.min_margin_level_pct
              << "%, min RR: " << cfg_.min_rr
              << ", aggregate_bars: " << cfg_.aggregate_bars
              << ", compound_sizing: " << (cfg_.compound_sizing ? "yes" : "no")
              << ", spread_mult: " << cfg_.spread_mult
              << ", commission_mult: " << cfg_.commission_mult
              << ", from_ms=" << cfg_.from_ms
              << ", to_ms="   << cfg_.to_ms
              << "\n\n";

    validation::AccountSnapshot account;
    account.balance              = cfg_.initial_balance;
    account.equity               = cfg_.initial_balance;
    account.free_margin          = cfg_.initial_balance;
    account.margin_used          = 0.0;
    account.leverage             = 500.0;
    account.min_margin_level_pct = cfg_.min_margin_level_pct;

    validation::AccountSnapshot sizing_account = account;
    if (!cfg_.compound_sizing) {
        sizing_account.balance = cfg_.initial_balance;
        sizing_account.equity  = cfg_.initial_balance;
    }

    std::unordered_map<uint64_t, TradeMeta> trade_meta;
    trade_meta.reserve(256);

    int    cur_day          = 0;
    double day_start_equity = account.equity;
    double day_min_equity   = account.equity;
    int64_t day_start_ms    = 0;

    std::map<int, double> month_pnl;

    auto flush_day = [&]() {
        if (cur_day == 0) return;
        if (day_start_equity > 0.0) {
            const double dd = (day_start_equity - day_min_equity)
                            / day_start_equity * 100.0;
            if (dd > rep.max_daily_dd_pct) {
                rep.max_daily_dd_pct = dd;
                rep.worst_day = day_str_from_ms(day_start_ms);
            }
        }
    };

    bool any_bar = true;
    while (any_bar) {
        int64_t min_ts = INT64_MAX;
        for (auto& s : slots) {
            if (s->has_bar && s->current_bar.timestamp < min_ts)
                min_ts = s->current_bar.timestamp;
        }
        if (min_ts == INT64_MAX) break;
        if (cfg_.to_ms > 0 && min_ts > cfg_.to_ms) break;

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

        if (cfg_.compound_sizing) {
            sizing_account.balance = account.balance;
            sizing_account.equity  = account.equity;
        }

        {
            const int dk = day_key_from_ms(min_ts);
            if (dk != cur_day) {
                flush_day();
                cur_day          = dk;
                day_start_equity = account.equity;
                day_min_equity   = account.equity;
                day_start_ms     = min_ts;
            } else {
                if (account.equity < day_min_equity)
                    day_min_equity = account.equity;
            }
        }

        for (std::size_t slot_idx = 0; slot_idx < slots.size(); ++slot_idx) {
            auto& s = slots[slot_idx];
            if (!s->has_bar || s->current_bar.timestamp != min_ts) continue;
            const core::Bar bar = s->current_bar;

            s->sig_history.push_back(bar);
            const std::size_t sig_keep = cfg_.context.max_zones_per_tf * 200;
            if (s->sig_history.size() > sig_keep)
                s->sig_history.erase(s->sig_history.begin(),
                                     s->sig_history.begin() +
                                        (s->sig_history.size() - sig_keep));

            ctx_accum_push(s->ctx_accum, s->ctx_accum_active, s->ctx_count, bar);
            if (s->ctx_count >= cfg_.aggregate_bars) {
                s->ctx_history.push_back(s->ctx_accum);
                s->ctx_accum_active = false;
                s->ctx_count = 0;
                const std::size_t ctx_keep = cfg_.context.max_zones_per_tf * 50;
                if (s->ctx_history.size() > ctx_keep)
                    s->ctx_history.erase(s->ctx_history.begin(),
                                         s->ctx_history.begin() +
                                            (s->ctx_history.size() - ctx_keep));
            }
            ++rep.bars_processed;

            auto mctx = s->ctx->analyze({}, s->ctx_history);
            auto sig  = s->pat->analyze(s->sig_history, mctx);

            if (sig.detected && skip_signal(sig, cfg_)) {
                sig.detected = false;
                ++rep.signals_skipped_type;
            }

            if (sig.detected) {
                ++rep.signals_total;
                auto vr = s->val->validate(sig, mctx, bar, sizing_account);
                if (vr.is_approved) {
                    const double req_margin =
                        (vr.order.volume * s->spec.contract_size) / s->spec.leverage;
                    const double new_used  = account.margin_used + req_margin;
                    const double new_level = (new_used > 0.0)
                                           ? (account.equity / new_used) * 100.0
                                           : 1e9;
                    if (new_level < cfg_.min_margin_level_pct) {
                        ++rep.orders_rejected;
                        ++rep.orders_margin_blocked;
                        continue;
                    }

                    const double entry = vr.order.entry_price;
                    const double sl    = vr.order.stop_loss;
                    const double tp2   = vr.order.take_profit_2;
                    double rr = 0.0;
                    if (vr.order.side == core::OrderSide::Buy) {
                        if (entry > sl) rr = (tp2 - entry) / (entry - sl);
                    } else {
                        if (sl > entry) rr = (entry - tp2) / (sl - entry);
                    }
                    if (rr < cfg_.min_rr) {
                        ++rep.orders_rejected;
                        ++rep.reject_rr;
                    } else {
                        ++rep.orders_approved;

                        // --- spread_mult ---
                        const int32_t base_spread = (bar.spread > 0)
                                                  ? bar.spread
                                                  : s->spec.spread_typical;
                        const int32_t spread_pts =
                            static_cast<int32_t>(base_spread * cfg_.spread_mult + 0.5);

                        const uint64_t pid =
                            s->pm->openPosition(vr.order, spread_pts, bar.timestamp);
                        const uint64_t key =
                            (static_cast<uint64_t>(slot_idx) << 32) |
                            static_cast<uint32_t>(pid);
                        TradeMeta meta;
                        meta.pattern_type = static_cast<int>(sig.type);
                        meta.pattern_side = (vr.order.side == core::OrderSide::Buy)
                                          ? 0 : 1;
                        trade_meta[key] = meta;
                        ++s->orders;
                        account.margin_used = new_used;
                        account.free_margin = account.equity - account.margin_used;
                    }
                } else {
                    ++rep.orders_rejected;
                    if (vr.reason == core::RejectReason::MarginCall)
                        ++rep.orders_margin_blocked;
                }
            }

            position::PositionAccountContext pac;
            pac.margin_level_pct = (account.margin_used > 0.0)
                                 ? (account.equity / account.margin_used) * 100.0
                                 : 1e9;
            pac.now_ms = bar.timestamp;
            auto events = s->pm->onBar(bar, pac);

            for (const auto& e : events) {
                const uint64_t key =
                    (static_cast<uint64_t>(slot_idx) << 32) |
                    static_cast<uint32_t>(e.position_id);
                auto it = trade_meta.find(key);

                if (e.type == position::PositionEventType::PartialClose) {
                    account.balance += e.net_pnl;
                    s->net_pnl      += e.net_pnl;
                    ++rep.partial_closes;
                    if (it != trade_meta.end())
                        it->second.accumulated_pnl += e.net_pnl;
                } else if (e.type == position::PositionEventType::FullClose) {
                    account.balance += e.net_pnl;
                    s->net_pnl      += e.net_pnl;
                    ++rep.full_closes;

                    const double trade_result = (it != trade_meta.end())
                                              ? it->second.accumulated_pnl + e.net_pnl
                                              : e.net_pnl;
                    const int ptype = (it != trade_meta.end()) ? it->second.pattern_type : 0;
                    const int pside = (it != trade_meta.end()) ? it->second.pattern_side : 0;
                    if (it != trade_meta.end()) trade_meta.erase(it);

                    if (trade_result > 0.0) {
                        ++rep.wins;
                        rep.total_win += trade_result;
                    } else if (trade_result < 0.0) {
                        ++rep.losses;
                        rep.total_loss += -trade_result;
                    }

                    auto find_pat = [&](int t, int sd) -> PortfolioReport::PatternResult* {
                        for (auto& b : rep.per_pattern)
                            if (b.type == t && b.side == sd) return &b;
                        rep.per_pattern.push_back({});
                        auto& b = rep.per_pattern.back();
                        b.type = t; b.side = sd;
                        return &b;
                    };
                    auto* pb = find_pat(ptype, pside);
                    ++pb->orders;
                    pb->net_pnl += trade_result;
                    if (trade_result > 0.0) {
                        ++pb->wins;
                        pb->total_win += trade_result;
                    } else if (trade_result < 0.0) {
                        ++pb->losses;
                        pb->total_loss += -trade_result;
                    }

                    const int yr = year_from_ms(e.time_ms);
                    auto find_yr = [&](int y) -> PortfolioReport::YearResult* {
                        for (auto& b : rep.per_year)
                            if (b.year == y) return &b;
                        rep.per_year.push_back({});
                        auto& b = rep.per_year.back();
                        b.year = y;
                        return &b;
                    };
                    auto* yb = find_yr(yr);
                    ++yb->orders;
                    yb->net_pnl += trade_result;
                    if (trade_result > 0.0) {
                        ++yb->wins;
                        yb->total_win += trade_result;
                    } else if (trade_result < 0.0) {
                        ++yb->losses;
                        yb->total_loss += -trade_result;
                    }

                    const int mk = month_key_from_ms(e.time_ms);
                    month_pnl[mk] += trade_result;
                }
            }

            if (!s->stream->next(s->current_bar)) {
                s->eof     = true;
                s->has_bar = false;
            }
        }
        if (rep.first_time_ms == 0) rep.first_time_ms = min_ts;
        rep.last_time_ms = min_ts;
    }

    flush_day();

    if (!month_pnl.empty()) {
        rep.total_months = month_pnl.size();
        double sum = 0.0;
        bool first = true;
        for (auto& [k, pnl] : month_pnl) {
            sum += pnl;
            if (pnl > 0.0)      ++rep.months_positive;
            else if (pnl < 0.0) ++rep.months_negative;
            else                ++rep.months_flat;
            if (first) {
                rep.worst_month_usd = pnl;
                rep.best_month_usd  = pnl;
                first = false;
            } else {
                if (pnl < rep.worst_month_usd) rep.worst_month_usd = pnl;
                if (pnl > rep.best_month_usd)  rep.best_month_usd  = pnl;
            }
        }
        rep.avg_month_usd = sum / static_cast<double>(rep.total_months);
        if (rep.initial_balance > 0.0) {
            rep.worst_month_pct = rep.worst_month_usd / rep.initial_balance * 100.0;
            rep.best_month_pct  = rep.best_month_usd  / rep.initial_balance * 100.0;
            rep.avg_month_pct   = rep.avg_month_usd   / rep.initial_balance * 100.0;
        }
    }

    rep.final_balance = account.balance;
    rep.net_pnl       = rep.final_balance - rep.initial_balance;
    rep.return_pct    = (rep.initial_balance > 0.0)
                      ? (rep.net_pnl / rep.initial_balance) * 100.0
                      : 0.0;

    const std::size_t closed = rep.wins + rep.losses;
    if (closed > 0)
        rep.win_rate = static_cast<double>(rep.wins) / closed * 100.0;
    if (rep.total_loss > 0.0)
        rep.profit_factor = rep.total_win / rep.total_loss;

    rep.per_instrument.reserve(slots.size());
    for (auto& s : slots) {
        rep.per_instrument.push_back({ s->symbol, s->orders, s->net_pnl });
    }
    std::sort(rep.per_instrument.begin(), rep.per_instrument.end(),
              [](const auto& a, const auto& b) { return a.net_pnl > b.net_pnl; });

    std::sort(rep.per_pattern.begin(), rep.per_pattern.end(),
              [](const auto& a, const auto& b) { return a.net_pnl > b.net_pnl; });

    std::sort(rep.per_year.begin(), rep.per_year.end(),
              [](const auto& a, const auto& b) { return a.year < b.year; });

    rep.ok = true;
    return rep;
}

} // namespace spartak::engine