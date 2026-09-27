// =============================================================================
//  SPARTAK :: core/InstrumentSpec.h
//  Параметры одного торгового инструмента.
//
//  Задаётся per-symbol и переопределяет дефолты из конфигов слоёв.
// =============================================================================
#pragma once
#include <cstdint>
#include <string>
namespace spartak::core {
// -----------------------------------------------------------------------------
// Категория инструмента (для auto-infer параметров).
// -----------------------------------------------------------------------------
enum class InstrumentCategory {
    Unknown,
    ForexMajor,      // EURUSD, GBPUSD, USDJPY, ...
    ForexCross,      // EURGBP, GBPJPY, ...
    Metal,           // XAUUSD, XAGUSD
    Index,           // US500, DE40, ...
    Crypto,          // BTCUSD, ETHUSD
    Stock            // BWXT, AAPL, ...
};
[[nodiscard]] const char* to_string(InstrumentCategory c) noexcept;
// -----------------------------------------------------------------------------
// Параметры инструмента.
// -----------------------------------------------------------------------------
struct InstrumentSpec {
    std::string        symbol;
    InstrumentCategory category = InstrumentCategory::Unknown;
    double             point          = 0.00001;   // шаг цены
    int32_t            digits         = 5;         // знаков после запятой
    double             contract_size  = 100'000.0; // единиц базового актива в 1 лоте
    double             min_lot        = 0.01;
    double             max_lot        = 1.0;
    double             lot_step       = 0.01;
    int32_t            spread_typical = 12;        // типичный спред в пунктах
    double             swap_long_points  = -7.0;
    double             swap_short_points = -2.0;
    double             commission_per_lot = 5.0;
    double             leverage           = 500.0;
    // --- Хелперы ---
    // Значение одного пункта в цене.
    [[nodiscard]] double point_value() const noexcept { return point; }
    // Стоимость 1 пункта на 1.0 лот в валюте депозита.
    [[nodiscard]] double point_cost_per_lot() const noexcept {
        return point * contract_size;
    }
};
} // namespace spartak::core