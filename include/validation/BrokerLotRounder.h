// =============================================================================
//  SPARTAK :: validation/BrokerLotRounder.h
//  Нормализация лота до брокерского шага с ограничением min/max.
//
//  Правило:
//    - floor до шага (никогда не превышать рассчитанный риск);
//    - обрезка по max_lot (не можем открыть больше максимума);
//    - если после нормализации lot < min_lot -> 0.0 (не открываем).
//
//  Дополнительно:
//    - is_valid_lot() — лежит ли значение на шаге;
//    - round_up()     — версия с округлением вверх (для агрессивных настроек).
// =============================================================================
#pragma once
#include <cstdint>
namespace spartak::validation {
// -----------------------------------------------------------------------------
// Конфиг.
// -----------------------------------------------------------------------------
struct LotRounderConfig {
    double min_lot  = 0.01;
    double max_lot  = 50.0;
    double lot_step = 0.01;
};
// -----------------------------------------------------------------------------
// BrokerLotRounder — stateless.
// -----------------------------------------------------------------------------
class BrokerLotRounder {
public:
    explicit BrokerLotRounder(LotRounderConfig cfg = {});
    // Округление ВНИЗ (безопасно, никогда не превысит риск).
    // Возвращает 0.0, если итоговый лот < min_lot.
    [[nodiscard]] double round_down(double raw_lot) const noexcept;
    // Округление ВВЕРХ (для агрессивных режимов — риск может превысить).
    [[nodiscard]] double round_up(double raw_lot) const noexcept;
    // Проверка, что lot лежит на шаге (с учётом floating-point).
    [[nodiscard]] bool is_valid_lot(double lot) const noexcept;
    // Сколько минимальных шагов помещается в raw_lot.
    [[nodiscard]] std::int64_t steps_in(double raw_lot) const noexcept;
    const LotRounderConfig& config() const noexcept { return cfg_; }
private:
    LotRounderConfig cfg_;
};
} // namespace spartak::validation