// =============================================================================
//  SPARTAK :: data/BarStream.h
//  Единая точка подачи баров в бэктест.
//
//  Два режима:
//    1) File      — из XFBarReader (реальный .bin)
//    2) Synthetic — генератор синусоиды + шума (smoke-тесты)
//
//  Интерфейс: next(Bar&) -> bool.
//  Дополнительно: push_back(Bar) — вернуть бар «назад» в поток (нужно
//  DataSanitizer'у, чтобы не терять регулярные бары, отобранные при
//  сканировании нерегулярного префикса).
// =============================================================================
#pragma once
#include "core/Types.h"
#include "data/XFBarReader.h"
#include <cstdint>
#include <cstddef>
#include <memory>
#include <random>
#include <deque>
#include <string>
namespace spartak::data {
enum class StreamMode {
    File,
    Synthetic
};
// -----------------------------------------------------------------------------
// BarStream
// -----------------------------------------------------------------------------
class BarStream {
public:
    // --- Конструкторы ---
    explicit BarStream(const std::string& path,
                       std::size_t chunk_bars = core::xfbar::DEFAULT_CHUNK_BARS);
    struct SyntheticParams {
        double      start_price   = 1.10000;
        double      point         = 0.00001;
        int32_t     spread_points = 12;
        std::size_t max_bars      = 1000;
        uint32_t    seed          = 1337;
    };
    explicit BarStream(const SyntheticParams& params);
    BarStream(const BarStream&)            = delete;
    BarStream& operator=(const BarStream&) = delete;
    // --- Основной интерфейс ---
    bool next(core::Bar& out);
    // Вернуть бар «назад» — он будет выдан при следующем next(),
    // перед чтением из источника. Работает как FIFO: порядок push_back
    // соответствует порядку выдачи.
    void push_back(const core::Bar& bar);
    // --- Метаданные ---
    StreamMode mode() const noexcept { return mode_; }
    const XFBarReader* reader() const noexcept { return reader_.get(); }
    int64_t total_bars() const noexcept;
    int64_t bars_read()  const noexcept { return emitted_; }
    bool    is_ok()      const noexcept { return ok_; }
    XFBarReaderStatus status() const noexcept;
private:
    StreamMode   mode_    = StreamMode::Synthetic;
    bool         ok_      = true;
    int64_t      emitted_ = 0;   // все бары, выданные наружу (включая push_back)
    int64_t      generated_ = 0; // бары, РЕАЛЬНО сгенерированные (только синтетика)
    // File
    std::unique_ptr<XFBarReader> reader_;
    // Synthetic
    SyntheticParams params_{};
    double          price_ = 0.0;
    std::mt19937    rng_;
    // Replay buffer (для push_back)
    std::deque<core::Bar> replay_;
    bool next_file(core::Bar& out);
    bool next_synthetic(core::Bar& out);
    core::Bar make_synthetic_bar();
};
} // namespace spartak::data