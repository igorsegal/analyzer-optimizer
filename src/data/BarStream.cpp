#include "data/BarStream.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace spartak::data {
// -----------------------------------------------------------------------------
// Конструктор: File
// -----------------------------------------------------------------------------
BarStream::BarStream(const std::string& path, std::size_t chunk_bars)
    : mode_(StreamMode::File),
      rng_(0)
{
    reader_ = std::make_unique<XFBarReader>(chunk_bars);
    ok_     = reader_->open(path);
}
// -----------------------------------------------------------------------------
// Конструктор: Synthetic
// -----------------------------------------------------------------------------
BarStream::BarStream(const SyntheticParams& params)
    : mode_(StreamMode::Synthetic),
      params_(params),
      price_(params.start_price),
      rng_(params.seed)
{
    if (params_.point <= 0.0)
        throw std::invalid_argument("BarStream: point must be > 0");
    if (params_.max_bars == 0)
        throw std::invalid_argument("BarStream: max_bars must be > 0");
    ok_ = true;
}
// -----------------------------------------------------------------------------
// push_back — возврат бара «назад» в поток (нужно DataSanitizer'у).
// Бар не увеличивает ни emitted_, ни generated_ — он уже был посчитан.
// -----------------------------------------------------------------------------
void BarStream::push_back(const core::Bar& bar) {
    replay_.push_back(bar);
}
// -----------------------------------------------------------------------------
// next — сначала replay-буфер, потом источник.
// -----------------------------------------------------------------------------
bool BarStream::next(core::Bar& out) {
    if (!ok_) return false;
    // 1. Возвращённые бары имеют приоритет
    if (!replay_.empty()) {
        out = replay_.front();
        replay_.pop_front();
        ++emitted_;
        return true;
    }
    // 2. Источник
    return (mode_ == StreamMode::File) ? next_file(out) : next_synthetic(out);
}
// -----------------------------------------------------------------------------
// next_file
// -----------------------------------------------------------------------------
bool BarStream::next_file(core::Bar& out) {
    if (!reader_ || !reader_->is_open()) return false;
    if (!reader_->read_next_bar(out))   return false;
    ++emitted_;
    ++generated_;   // в файловом режиме generated_ = emitted_, для единообразия
    return true;
}
// -----------------------------------------------------------------------------
// next_synthetic
//
// Фаза синусоиды и timestamp зависят от generated_ (только реально
// сгенерированные бары), а не от emitted_ (который учитывает replay).
// Так push_back не сдвигает синусоиду.
// -----------------------------------------------------------------------------
bool BarStream::next_synthetic(core::Bar& out) {
    if (generated_ >= static_cast<int64_t>(params_.max_bars)) return false;
    out = make_synthetic_bar();
    ++generated_;
    ++emitted_;
    return true;
}
core::Bar BarStream::make_synthetic_bar() {
    const double wave   = std::sin(static_cast<double>(generated_) / 12.0);
    std::uniform_real_distribution<double> noise(-2.5, 2.5);
    const double drift  = (wave * 15.0 + noise(rng_)) * params_.point;
    const double open   = price_;
    const double close  = price_ + drift;
    const double hi_off = (std::fabs(noise(rng_)) + 2.0) * params_.point;
    const double lo_off = (std::fabs(noise(rng_)) + 2.0) * params_.point;
    const double high   = std::max(open, close) + hi_off;
    const double low    = std::min(open, close) - lo_off;
    price_ = close;
    core::Bar b;
    b.timestamp   = 1'700'000'000'000LL
                  + static_cast<int64_t>(generated_) * 300'000LL;
    b.open        = open;
    b.high        = high;
    b.low         = low;
    b.close       = close;
    b.tick_volume = 100;
    b.spread      = params_.spread_points;
    return b;
}
// -----------------------------------------------------------------------------
// Метаданные
// -----------------------------------------------------------------------------
int64_t BarStream::total_bars() const noexcept {
    if (mode_ == StreamMode::File) {
        return reader_ ? reader_->total_bars() : 0;
    }
    return static_cast<int64_t>(params_.max_bars);
}
XFBarReaderStatus BarStream::status() const noexcept {
    if (mode_ != StreamMode::File || !reader_)
        return XFBarReaderStatus::Ok;
    return reader_->status();
}
} // namespace spartak::data