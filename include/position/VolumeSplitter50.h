// =============================================================================
//  SPARTAK :: position/VolumeSplitter50.h
//  Разбиение объёма позиции для частичного закрытия (50/50, 30/30/40 и т.д.).
//
//  Используется на TP1: закрываем `pct` от ИЗНАЧАЛЬНОГО объёма,
//  остаток остаётся открытым до TP2 или SL.
//
//  Возвращает:
//    - close_lot    — объём к закрытию;
//    - remain_lot   — что остаётся в позиции.
//
//  Оба значения нормализуются до шага лота; если остаток после split
//  уходит ниже min_lot — закрываем ВСЁ (split невозможен).
// =============================================================================
#pragma once
#include <cstdint>
namespace spartak::position {
// -----------------------------------------------------------------------------
// Конфиг.
// -----------------------------------------------------------------------------
struct VolumeSplitterConfig {
    double partial_pct  = 0.50;    // доля от начального объёма
    double min_lot      = 0.01;
    double lot_step     = 0.01;
};
// -----------------------------------------------------------------------------
// Результат разбиения.
// -----------------------------------------------------------------------------
struct SplitResult {
    bool   ok          = false;   // удалось ли корректно разбить
    double close_lot   = 0.0;     // закрываем сейчас
    double remain_lot  = 0.0;     // остаётся открытым
    bool   full_close  = false;   // если разбить нельзя -> всё закрываем
};
// -----------------------------------------------------------------------------
// VolumeSplitter50 — stateless.
// -----------------------------------------------------------------------------
class VolumeSplitter50 {
public:
    explicit VolumeSplitter50(VolumeSplitterConfig cfg = {});
    // split(initial, remaining)
    //   initial   — начальный объём позиции (для расчёта partial_pct)
    //   remaining — текущий остаток (может быть уже уменьшен предыдущими partial)
    //               В типовом сценарии remaining == initial.
    [[nodiscard]] SplitResult split(double initial, double remaining) const noexcept;
    const VolumeSplitterConfig& config() const noexcept { return cfg_; }
private:
    VolumeSplitterConfig cfg_;
};
} // namespace spartak::position