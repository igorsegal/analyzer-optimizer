#include "context/FractalPointDetector.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace spartak::context {
// -----------------------------------------------------------------------------
// Конструктор с валидацией
// -----------------------------------------------------------------------------
FractalPointDetector::FractalPointDetector(int radius)
    : radius_(radius) {
    if (radius_ < 1)
        throw std::invalid_argument("FractalPointDetector: radius must be >= 1");
}
// -----------------------------------------------------------------------------
// find — фрактальный поиск экстремумов.
//
// Алгоритм:
//   для i от radius до n-radius-1:
//     High-фрактал: bar[i].high СТРОГО > bar[j].high для всех j в [i-r, i+r], j != i
//     Low-фрактал:  bar[i].low  СТРОГО < bar[j].low  для всех j в [i-r, i+r], j != i
//
// Защита: бары с NaN/Inf или отрицательными ценами пропускаются (не считаются
// ни кандидатами, ни «соседями» — но для простоты при валидации входа
// фильтруем только сам бар-кандидат; реальные XFBAR-данные чистые).
//
// Результат — в хронологическом порядке (по index).
// -----------------------------------------------------------------------------
std::vector<Extremum>
FractalPointDetector::find(const std::vector<core::Bar>& bars) const {
    std::vector<Extremum> out;
    const std::size_t n = bars.size();
    const int r = radius_;
    // Слишком короткая серия — фрактал не построить
    if (n < static_cast<std::size_t>(2 * r + 1)) return out;
    out.reserve(n / 4);   // эвристика по памяти
    for (std::size_t i = static_cast<std::size_t>(r);
         i + static_cast<std::size_t>(r) < n;
         ++i)
    {
        const double h = bars[i].high;
        const double l = bars[i].low;
        // Защита от битых данных
        if (!std::isfinite(h) || !std::isfinite(l) || h <= 0.0 || l <= 0.0)
            continue;
        bool is_high = true;
        bool is_low  = true;
        for (int k = -r; k <= r; ++k) {
            if (k == 0) continue;
            const std::size_t j = static_cast<std::size_t>(
                static_cast<long long>(i) + k);
            if (bars[j].high >= h) is_high = false;
            if (bars[j].low  <= l) is_low  = false;
            if (!is_high && !is_low) break;   // ранний выход
        }
        if (is_high) {
            out.push_back({Extremum::Kind::High, i, bars[i].timestamp, h});
        }
        if (is_low) {
            out.push_back({Extremum::Kind::Low,  i, bars[i].timestamp, l});
        }
    }
    // Гарантируем хронологический порядок (мы и так добавляем по возрастанию i,
    // но sort делает контракт явным и защищает от будущих правок).
    std::sort(out.begin(), out.end(),
              [](const Extremum& a, const Extremum& b) {
                  return a.index < b.index;
              });
    return out;
}
} // namespace spartak::context