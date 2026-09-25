#include "position/EmergencyCloseHandler.h"
#include "core/Constants.h"
#include <cmath>
namespace spartak::position {
const char* to_string(EmergencyReason r) noexcept {
    switch (r) {
        case EmergencyReason::None:           return "None";
        case EmergencyReason::MarginCall:     return "MarginCall";
        case EmergencyReason::DrawdownLimit:  return "DrawdownLimit";
        case EmergencyReason::MaxHoldingTime: return "MaxHoldingTime";
    }
    return "Unknown";
}
EmergencyCloseHandler::EmergencyCloseHandler(EmergencyConfig cfg)
    : cfg_(cfg) {}
// -----------------------------------------------------------------------------
// check — триггеры в порядке приоритета:
//   1) MarginCall — критично для счёта
//   2) Drawdown   — критично для одной позиции
//   3) MaxHolding — гигиена (swap)
// -----------------------------------------------------------------------------
EmergencyDecision EmergencyCloseHandler::check(const EmergencyContext& ctx) const noexcept {
    EmergencyDecision r;
    // 1. MarginCall
    if (cfg_.margin_check_enabled &&
        ctx.margin_level_pct > 0.0 &&
        ctx.margin_level_pct < cfg_.min_margin_level_pct)
    {
        r.should_close = true;
        r.reason       = EmergencyReason::MarginCall;
        r.value        = ctx.margin_level_pct;
        return r;
    }
    // 2. Drawdown
    if (cfg_.drawdown_check_enabled && ctx.risk_money > 0.0) {
        // Если pnl отрицательный и превышает risk_money * max_drawdown_pct / 100
        const double loss = -ctx.current_pnl;
        if (loss > 0.0) {
            const double dd_pct = (loss / ctx.risk_money) * 100.0;
            if (dd_pct > cfg_.max_drawdown_pct) {
                r.should_close = true;
                r.reason       = EmergencyReason::DrawdownLimit;
                r.value        = dd_pct;
                return r;
            }
        }
    }
    // 3. MaxHolding
    if (cfg_.holding_check_enabled &&
        ctx.open_time_ms > 0 && ctx.now_ms > ctx.open_time_ms)
    {
        const int64_t age_ms = ctx.now_ms - ctx.open_time_ms;
        const int64_t max_ms = static_cast<int64_t>(cfg_.max_holding_days)
                             * core::numeric::MS_PER_DAY;
        if (age_ms > max_ms) {
            const double days = static_cast<double>(age_ms) / core::numeric::MS_PER_DAY;
            r.should_close = true;
            r.reason       = EmergencyReason::MaxHoldingTime;
            r.value        = days;
            return r;
        }
    }
    return r;
}
} // namespace spartak::position