// =============================================================================
//  SPARTAK :: context/FractalPointDetector.h
//  Фрактальный поиск локальных пиков и впадин.
//
//  Логика:
//    High-фрактал в баре i, если bar[i].high СТРОГО больше
//    всех high в окне [i-radius, i+radius], исключая сам i.
//
//    Low-фрактал — аналогично по bar[i].low.
//
//  Результат — хронологический список Extremum-ов.
//  Detector НЕ знает ни про тренд, ни про зоны — только «здесь был пик».
// =============================================================================
#pragma once
#include "core/Types.h"
#include <cstdint>
#include <cstddef>
#include <vector>
namespace spartak::context {
// -----------------------------------------------------------------------------
// Extremum — найденный локальный пик или впадина.
// -----------------------------------------------------------------------------
struct Extremum {
    enum class Kind { High, Low };
    Kind        kind      = Kind::High;
    std::size_t index     = 0;      // индекс бара в исходном массиве
    int64_t     timestamp = 0;      // время бара (Unix ms)
    double      price     = 0.0;    // high для High, low для Low
};
// -----------------------------------------------------------------------------
// FractalPointDetector — stateless-класс, только радиус конфигурируется.
// -----------------------------------------------------------------------------
class FractalPointDetector {
public:
    // radius — количество баров влево и вправо для сравнения.
    // radius = 2 — «стандартный» фрактал Билла Вильямса.
    explicit FractalPointDetector(int radius = 2);
    // Главный вход: возвращает фракталы в хронологическом порядке.
    [[nodiscard]] std::vector<Extremum>
    find(const std::vector<core::Bar>& bars) const;
    int radius() const noexcept { return radius_; }
private:
    int radius_;
};
} // namespace spartak::context