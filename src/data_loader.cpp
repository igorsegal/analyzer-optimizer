#include "../include/DataLoader.h"
#include <fstream>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;
using namespace AHexa;

std::vector<InstrumentInfo> DataLoader::scan(const std::string& rootDir) {
    std::vector<InstrumentInfo> list;
    for (const auto& entry : fs::recursive_directory_iterator(rootDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".bin") {
            std::string fname = entry.path().filename().string();
            // Только файлы вида *_M5.bin (игнорируем _IND)
            if (fname.find("_M5.bin") != std::string::npos) {
                InstrumentInfo info;
                info.filePath = entry.path().string();
                // Имя символа — до "_M5"
                info.symbol = fname.substr(0, fname.find("_M5"));
                list.push_back(info);
            }
        }
    }
    return list;
}

std::vector<Bar> DataLoader::loadBars(const InstrumentInfo& info) {
    std::vector<Bar> bars;
    std::ifstream file(info.filePath, std::ios::binary);
    if (!file) return bars;

    char magic[8];
    int32_t version, record_size, period_seconds, digits;
    double point;
    int64_t bar_count, first_time, last_time;
    int32_t symbol_len;
    file.read(magic, 8);
    if (std::memcmp(magic, "XFBAR001", 8) != 0) return bars;
    file.read(reinterpret_cast<char*>(&version), 4);
    file.read(reinterpret_cast<char*>(&record_size), 4);
    file.read(reinterpret_cast<char*>(&period_seconds), 4);
    file.read(reinterpret_cast<char*>(&digits), 4);
    file.read(reinterpret_cast<char*>(&point), 8);
    file.read(reinterpret_cast<char*>(&bar_count), 8);
    file.read(reinterpret_cast<char*>(&first_time), 8);
    file.read(reinterpret_cast<char*>(&last_time), 8);
    file.read(reinterpret_cast<char*>(&symbol_len), 4);
    file.seekg(symbol_len, std::ios::cur);

    bars.resize(bar_count);
    for (int64_t i = 0; i < bar_count; ++i) {
        file.read(reinterpret_cast<char*>(&bars[i].time), 8);
        file.read(reinterpret_cast<char*>(&bars[i].open), 8);
        file.read(reinterpret_cast<char*>(&bars[i].high), 8);
        file.read(reinterpret_cast<char*>(&bars[i].low), 8);
        file.read(reinterpret_cast<char*>(&bars[i].close), 8);
        file.read(reinterpret_cast<char*>(&bars[i].tick_volume), 8);
        file.read(reinterpret_cast<char*>(&bars[i].spread), 4);
        file.read(reinterpret_cast<char*>(&bars[i].real_volume), 8);
    }
    return bars;
}