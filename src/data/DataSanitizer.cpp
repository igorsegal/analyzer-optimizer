#include "data/DataSanitizer.h"
#include <stdexcept>
#include <vector>
namespace spartak::data {
// -----------------------------------------------------------------------------
// Конструктор с валидацией
// -----------------------------------------------------------------------------
DataSanitizer::DataSanitizer(int64_t max_gap_ms, int confirm_bars)
    : max_gap_ms_(max_gap_ms), confirm_bars_(confirm_bars) {
    if (max_gap_ms_ <= 0)
        throw std::invalid_argument("DataSanitizer: max_gap_ms must be > 0");
    if (confirm_bars_ < 1)
        throw std::invalid_argument("DataSanitizer: confirm_bars must be >= 1");
}
// -----------------------------------------------------------------------------
// run — читает поток до подтверждения регулярного старта.
//
// Схема:
//   prev = первый бар
//   cur  = следующий бар
//   delta = cur.timestamp - prev.timestamp
//
//   Если delta <= max_gap_ms:
//       - первый раз в run: инициализируем буфер regular_ = { prev }
//       - добавляем cur в regular_
//       - streak++
//   Иначе:
//       - если был активный run — сбрасываем буфер
//       - новый prev = cur
//
//   Как только streak >= confirm_bars — успех:
//     - regular_ содержит ровно (confirm_bars + 1) баров
//     - bars_skipped = bars_scanned - regular_.size()
//     - возвращаем ВСЕ бары regular_ в поток через push_back (в порядке FIFO)
//     - возвращаем ok=true
//
//   Если поток закончился — ok=false, всё что прочитано, потеряно
//   (возвращать нечего, strategy не сможет работать на нерегулярных данных).
// -----------------------------------------------------------------------------
SanitizeReport DataSanitizer::run(BarStream& stream) const {
    SanitizeReport rep;
    core::Bar prev;
    if (!stream.next(prev)) {
        return rep;   // пустой поток
    }
    ++rep.bars_scanned;
    std::vector<core::Bar> regular;   // накопление «регулярных» баров
    regular.reserve(confirm_bars_ + 1);
    int streak = 0;
    core::Bar cur;
    while (stream.next(cur)) {
        ++rep.bars_scanned;
        const int64_t delta = cur.timestamp - prev.timestamp;
        if (delta > rep.max_gap_seen_ms) rep.max_gap_seen_ms = delta;
        if (delta > 0 && delta <= max_gap_ms_) {
            if (streak == 0) {
                // Начало нового run — prev становится первым регулярным баром
                regular.clear();
                regular.push_back(prev);
                rep.first_regular_timestamp = prev.timestamp;
            }
            regular.push_back(cur);
            ++streak;
        } else {
            // Разрыв — сбрасываем run
            streak = 0;
            regular.clear();
            rep.first_regular_timestamp = 0;
        }
        if (streak >= confirm_bars_) {
            rep.ok = true;
            rep.bars_skipped = rep.bars_scanned - static_cast<int64_t>(regular.size());
            if (rep.bars_skipped < 0) rep.bars_skipped = 0;
            // Возвращаем все регулярные бары назад в поток (FIFO)
            for (const auto& b : regular) {
                stream.push_back(b);
            }
            return rep;
        }
        prev = cur;
    }
    // Поток закончился, регулярности не нашли.
    rep.ok = false;
    rep.bars_skipped = rep.bars_scanned;
    return rep;
}
} // namespace spartak::data