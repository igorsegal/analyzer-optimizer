// =============================================================================
//  SPARTAK :: context/HighExtractor.h
//  Извлекает только High-экстремумы из общего списка фракталов.
//
//  Зачем: StructureValidatorHHHL и Zone-калькуляторы работают отдельно
//  с последовательностями High и Low. Разделять — дёшево и явно.
// =============================================================================
#pragma once
#include "context/FractalPointDetector.h"
#include <vector>
namespace spartak::context {
class HighExtractor {
public:
    // Возвращает только High-экстремумы (в исходном порядке).
    [[nodiscard]] static std::vector<Extremum>
    extract(const std::vector<Extremum>& extrema);
};
} // namespace spartak::context