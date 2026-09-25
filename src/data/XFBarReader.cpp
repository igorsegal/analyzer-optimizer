#include "data/XFBarReader.h"
#include <cstring>
#include <stdexcept>
namespace spartak::data {
bool XFBarHeader::magic_ok() const noexcept {
    return std::memcmp(magic, core::xfbar::MAGIC.data(), core::xfbar::MAGIC_SIZE) == 0;
}
const char* to_string(XFBarReaderStatus s) noexcept {
    switch (s) {
        case XFBarReaderStatus::Ok:             return "Ok";
        case XFBarReaderStatus::FileNotOpen:    return "FileNotOpen";
        case XFBarReaderStatus::HeaderTruncated:return "HeaderTruncated";
        case XFBarReaderStatus::BadMagic:       return "BadMagic";
        case XFBarReaderStatus::BadVersion:     return "BadVersion";
        case XFBarReaderStatus::BadRecordSize:  return "BadRecordSize";
        case XFBarReaderStatus::BadSymbolLen:   return "BadSymbolLen";
        case XFBarReaderStatus::ZeroBars:       return "ZeroBars";
        case XFBarReaderStatus::TruncatedData:  return "TruncatedData";
    }
    return "Unknown";
}
#pragma pack(push, 1)
struct RawRecord {
    int64_t time;
    double  open;
    double  high;
    double  low;
    double  close;
    int64_t tick_volume;
    int32_t spread;
    int64_t real_volume;
};
#pragma pack(pop)
static_assert(sizeof(RawRecord) == core::xfbar::RECORD_SIZE,
              "RawRecord must be exactly 60 bytes");
XFBarReader::XFBarReader(std::size_t chunk_bars)
    : chunk_bars_(chunk_bars) {
    if (chunk_bars_ == 0)
        throw std::invalid_argument("XFBarReader: chunk_bars must be > 0");
    buffer_.resize(chunk_bars_);
}
XFBarReader::~XFBarReader() {
    if (file_.is_open()) file_.close();
}
void XFBarReader::reset() {
    header_        = {};
    buffer_index_  = 0;
    buffer_count_  = 0;
    total_read_    = 0;
    status_        = XFBarReaderStatus::Ok;
}
void XFBarReader::close() {
    if (file_.is_open()) file_.close();
    // НЕ трогаем status_ — чтобы причина ошибки open() не затиралась
}
bool XFBarReader::open(const std::string& path) {
    if (file_.is_open()) file_.close();
    reset();
    file_.open(path, std::ios::binary);
    if (!file_.is_open()) {
        status_ = XFBarReaderStatus::FileNotOpen;
        return false;
    }
    // --- Фиксированный заголовок (60 байт) ---
    file_.read(reinterpret_cast<char*>(&header_.magic),          core::xfbar::MAGIC_SIZE);
    file_.read(reinterpret_cast<char*>(&header_.version),        sizeof(int32_t));
    file_.read(reinterpret_cast<char*>(&header_.record_size),    sizeof(int32_t));
    file_.read(reinterpret_cast<char*>(&header_.period_seconds), sizeof(int32_t));
    file_.read(reinterpret_cast<char*>(&header_.digits),         sizeof(int32_t));
    file_.read(reinterpret_cast<char*>(&header_.point),          sizeof(double));
    file_.read(reinterpret_cast<char*>(&header_.bar_count),      sizeof(int64_t));
    file_.read(reinterpret_cast<char*>(&header_.first_time),     sizeof(int64_t));
    file_.read(reinterpret_cast<char*>(&header_.last_time),      sizeof(int64_t));
    file_.read(reinterpret_cast<char*>(&header_.symbol_len),     sizeof(int32_t));
    // ПРАВИЛЬНАЯ проверка: file_.fail() взводится, если хоть один read не дочитал
    if (file_.fail()) {
        status_ = XFBarReaderStatus::HeaderTruncated;
        file_.close();
        return false;
    }
    if (!header_.magic_ok()) {
        status_ = XFBarReaderStatus::BadMagic;
        file_.close();
        return false;
    }
    if (header_.version != core::xfbar::VERSION) {
        status_ = XFBarReaderStatus::BadVersion;
        file_.close();
        return false;
    }
    if (header_.record_size != static_cast<int32_t>(core::xfbar::RECORD_SIZE)) {
        status_ = XFBarReaderStatus::BadRecordSize;
        file_.close();
        return false;
    }
    if (header_.symbol_len < 0 || header_.symbol_len > core::xfbar::MAX_SYMBOL_LEN) {
        status_ = XFBarReaderStatus::BadSymbolLen;
        file_.close();
        return false;
    }
    if (header_.bar_count <= 0) {
        status_ = XFBarReaderStatus::ZeroBars;
        file_.close();
        return false;
    }
    // --- Имя символа переменной длины ---
    if (header_.symbol_len > 0) {
        header_.symbol.resize(static_cast<std::size_t>(header_.symbol_len));
        file_.read(header_.symbol.data(), header_.symbol_len);
        if (file_.fail()) {
            status_ = XFBarReaderStatus::HeaderTruncated;
            file_.close();
            return false;
        }
    }
    // --- Первый чанк данных ---
    if (!load_next_chunk()) {
        file_.close();
        return false;
    }
    status_ = XFBarReaderStatus::Ok;
    return true;
}
bool XFBarReader::load_next_chunk() {
    if (!file_.is_open()) {
        status_ = XFBarReaderStatus::FileNotOpen;
        return false;
    }
    if (total_read_ >= header_.bar_count) {
        return false;
    }
    const int64_t remaining = header_.bar_count - total_read_;
    const std::size_t to_read =
        static_cast<std::size_t>(std::min<int64_t>(remaining,
                                  static_cast<int64_t>(chunk_bars_)));
    std::vector<RawRecord> raw(to_read);
    file_.read(reinterpret_cast<char*>(raw.data()),
               static_cast<std::streamsize>(to_read * sizeof(RawRecord)));
    const std::size_t read_bytes   = static_cast<std::size_t>(file_.gcount());
    const std::size_t read_records = read_bytes / sizeof(RawRecord);
    if (read_records == 0) {
        status_ = XFBarReaderStatus::TruncatedData;
        return false;
    }
    for (std::size_t i = 0; i < read_records; ++i) {
        buffer_[i].timestamp   = raw[i].time * 1000LL;   // XFBAR хранит секунды, приводим к мс
        buffer_[i].open        = raw[i].open;
        buffer_[i].high        = raw[i].high;
        buffer_[i].low         = raw[i].low;
        buffer_[i].close       = raw[i].close;
        buffer_[i].tick_volume = raw[i].tick_volume;
        buffer_[i].spread      = raw[i].spread;
    }
    buffer_index_ = 0;
    buffer_count_ = read_records;
    total_read_  += static_cast<int64_t>(read_records);
    return true;
}
bool XFBarReader::read_next_bar(core::Bar& out) {
    if (buffer_index_ >= buffer_count_) {
        if (!load_next_chunk()) return false;
    }
    out = buffer_[buffer_index_++];
    return true;
}
} // namespace spartak::data