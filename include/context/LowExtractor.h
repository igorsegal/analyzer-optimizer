// =============================================================================
//  SPARTAK :: context/LowExtractor.h
//  Извлекает только Low-экстремумы из общего списка фракталов.
//
//  Симметричен HighExtractor. Используется StructureValidatorHHHL,
//  LU/PU-калькуляторами и TrendBiasEvaluator.
// =============================================================================
#pragma once
#include "context/FractalPointDetector.h"
#include <vector>
namespace spartak::context {
class LowExtractor {
public:
    // Возвращает только Low-экстремумы (в исходном порядке).
    [[nodiscard]] static std::vector<Extremum>
    extract(const std::vector<Extremum>& extrema);
};
} // namespace spartak::context