// =============================================================================
//  SPARTAK :: data/XFBarReader.h
//  Чтение бинарных файлов формата XFBAR001.
//
//  Формат файла:
//    [Fixed header 60 bytes]
//      char    magic[8]            "XFBAR001"
//      int32   version             1
//      int32   record_size         60
//      int32   period_seconds      300 / 900 / ...
//      int32   digits              5
//      double  point               0.00001
//      int64   bar_count           N
//      int64   first_time          Unix ms
//      int64   last_time           Unix ms
//      int32   symbol_len          L
//    [Symbol name L bytes, UTF-8]
//    [N * Record 60 bytes]
//      int64   time
//      double  open
//      double  high
//      double  low
//      double  close
//      int64   tick_volume
//      int32   spread              (в пунктах)
//      int64   real_volume
//
//  Читатель потоковый: держит буфер chunk_bars записей и подгружает
//  следующую порцию по мере исчерпания. RAM не растёт с размером файла.
// =============================================================================

#pragma once

#include "core/Types.h"
#include "core/Constants.h"
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <fstream>

namespace spartak::data {

// -----------------------------------------------------------------------------
// Заголовок XFBAR (фиксированная часть + имя символа).
// -----------------------------------------------------------------------------
struct XFBarHeader {
    char     magic[core::xfbar::MAGIC_SIZE] = {}; // "XFBAR001"
    int32_t  version        = 0;
    int32_t  record_size    = 0;
    int32_t  period_seconds = 0;
    int32_t  digits         = 0;
    double   point          = 0.0;
    int64_t  bar_count      = 0;
    int64_t  first_time     = 0;   // Unix ms
    int64_t  last_time      = 0;   // Unix ms
    int32_t  symbol_len     = 0;
    std::string symbol;            // имя символа (переменная длина)

    bool magic_ok() const noexcept;
};

// -----------------------------------------------------------------------------
// Ошибки, с которыми может завершиться open().
// -----------------------------------------------------------------------------
enum class XFBarReaderStatus {
    Ok = 0,
    FileNotOpen,
    HeaderTruncated,
    BadMagic,
    BadVersion,
    BadRecordSize,
    BadSymbolLen,
    ZeroBars,
    TruncatedData
};

[[nodiscard]] const char* to_string(XFBarReaderStatus s) noexcept;

// -----------------------------------------------------------------------------
// XFBarReader — потоковое чтение XFBAR-файла.
// -----------------------------------------------------------------------------
class XFBarReader {
public:
    explicit XFBarReader(std::size_t chunk_bars = core::xfbar::DEFAULT_CHUNK_BARS);

    XFBarReader(const XFBarReader&)            = delete;
    XFBarReader& operator=(const XFBarReader&) = delete;

    ~XFBarReader();

    // --- Открытие / закрытие ---
    bool open(const std::string& path);
    void close();

    // --- Потоковое чтение ---
    // true — бар записан в out, false — EOF или ошибка (см. status()).
    bool read_next_bar(core::Bar& out);

    // --- Метаданные ---
    bool is_open()         const noexcept { return file_.is_open(); }
    XFBarReaderStatus status() const noexcept { return status_; }
    const XFBarHeader& header() const noexcept { return header_; }
    const std::string& symbol() const noexcept { return header_.symbol; }
    int64_t total_bars()    const noexcept { return header_.bar_count; }
    int64_t bars_read()     const noexcept { return total_read_; }

private:
    std::size_t        chunk_bars_;
    std::ifstream      file_;
    XFBarHeader        header_;
    XFBarReaderStatus  status_ = XFBarReaderStatus::Ok;

    // Буфер записей на диске (сырой) и распакованный поток Bar
    std::vector<core::Bar> buffer_;
    std::size_t            buffer_index_ = 0;
    std::size_t            buffer_count_ = 0;
    int64_t                total_read_   = 0;

    bool load_next_chunk();
    void reset();
};

} // namespace spartak::data