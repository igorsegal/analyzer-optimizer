#include "core/Config.h"

namespace spartak::core {

AppConfig make_default_config() {
    AppConfig cfg;

    cfg.account.equity      = cfg.account.balance;
    cfg.account.free_margin = cfg.account.balance;
    cfg.account.margin_used = 0.0;

    cfg.position.point              = cfg.validation.point;
    cfg.position.contract_size      = cfg.validation.contract_size;
    cfg.position.min_lot            = cfg.validation.min_lot;
    cfg.position.lot_step           = cfg.validation.lot_step;
    cfg.position.commission_per_lot = cfg.account.commission_per_lot;

    cfg.feed.use_synthetic = false;

    cfg.engine.max_bars = cfg.feed.max_bars > 0
                        ? cfg.feed.max_bars
                        : defaults::MAX_BARS;

    return cfg;
}

const char* to_string(RejectReason r) noexcept {
    switch (r) {
        case RejectReason::None:            return "None";
        case RejectReason::InvalidSignal:   return "InvalidSignal";
        case RejectReason::SpreadTooHigh:   return "SpreadTooHigh";
        case RejectReason::TrendConflict:   return "TrendConflict";
        case RejectReason::InvalidStop:     return "InvalidStop";
        case RejectReason::ZeroDistance:    return "ZeroDistance";
        case RejectReason::BalanceTooLow:   return "BalanceTooLow";
        case RejectReason::BelowMinLot:     return "BelowMinLot";
        case RejectReason::MarginTooLow:    return "MarginTooLow";
        case RejectReason::MarginCall:      return "MarginCall";
        case RejectReason::VolumeClamped:   return "VolumeClamped";
        case RejectReason::NoContext:       return "NoContext";
        case RejectReason::SessionClosed:   return "SessionClosed";
        case RejectReason::SlippageTooHigh: return "SlippageTooHigh";
    }
    return "Unknown";
}

} // namespace spartak::core