// =============================================================================
//  SPARTAK :: position/SwapAccrualTracker.h
//  Учёт свопов при переходе через полночь (00:00 UTC).
//
//  По методике своп начисляется ОДИН РАЗ за каждую ночь, которую позиция
//  провела открытой. Считаем по UTC-дню:
//
//    day = floor(unix_ms / MS_PER_DAY)
//
//  last_swap_day — день последнего начисления. Если now_day > last_swap_day,
//  начисляем своп за (now_day - last_swap_day) ночей.
//
//  Своп задаётся в пунктах за 1.0 лот (может быть отрицательным — списание).
//  Деньги = points * point * contract_size * volume * nights.
// =============================================================================
#pragma once
#include "core/Types.h"
#include "core/Constants.h"
#include <cstdint>
namespace spartak::position {
// -----------------------------------------------------------------------------
// Конфиг.
// -----------------------------------------------------------------------------
struct SwapConfig {
    double swap_long_points   = -7.0;    // своп за ночь на BUY, в пунктах/лот
    double swap_short_points  = -2.0;    // своп за ночь на SELL
    double point              = 0.00001;
    double contract_size      = 100'000.0;
};
// -----------------------------------------------------------------------------
// Результат начисления.
// -----------------------------------------------------------------------------
struct SwapAccrual {
    bool    accrued  = false;      // был ли факт начисления
    int     nights   = 0;          // сколько ночей добавилось
    double  money    = 0.0;        // своп в деньгах (со знаком)
    int64_t new_day  = 0;          // обновлённый last_swap_day
};
// -----------------------------------------------------------------------------
// SwapAccrualTracker — stateless.
// -----------------------------------------------------------------------------
class SwapAccrualTracker {
public:
    explicit SwapAccrualTracker(SwapConfig cfg = {});
    // Определить, сколько ночей прошло и посчитать своп.
    //   side            — сторона позиции
    //   volume          — текущий объём (в лотах)
    //   open_ms         — время открытия (для инициализации при старте)
    //   now_ms          — текущее время
    //   last_swap_day   — предыдущий «день свопа»; -1 если ещё не было
    [[nodiscard]] SwapAccrual compute(core::OrderSide side,
                                      double volume,
                                      int64_t last_swap_day,
                                      int64_t now_ms) const noexcept;
    // День (UTC) по timestamp в мс.
    [[nodiscard]] static int64_t day_of(int64_t timestamp_ms) noexcept;
    const SwapConfig& config() const noexcept { return cfg_; }
private:
    SwapConfig cfg_;
};
} // namespace spartak::position