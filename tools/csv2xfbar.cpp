// =============================================================================
//  SPARTAK :: tools/csv2xfbar.cpp
//  Конвертер CSV истории -> бинарный формат XFBAR001.
//
//  Вход:  CSV с колонками
//         <TIME>,<OPEN>,<HIGH>,<LOW>,<CLOSE>,<TICK_VOLUME>,<SPREAD>,<REAL_VOLUME>
//         TIME в формате "YYYY.MM.DD HH:MM".
//
//  Выход: XFBAR001 .bin (60-байтный заголовок + имя символа + 60-байтные записи).
//         Время в записях — Unix SECONDS (как в существующих .bin).
//
//  Использование:
//    csv2xfbar <in.csv> <out.bin> <symbol> [period_sec] [digits] [point]
// =============================================================================
#define _CRT_SECURE_NO_WARNINGS
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#pragma pack(push, 1)
struct XFBarHeaderFixed {
    char     magic[8];
    int32_t  version;
    int32_t  record_size;
    int32_t  period_seconds;
    int32_t  digits;
    double   point;
    int64_t  bar_count;
    int64_t  first_time;
    int64_t  last_time;
    int32_t  symbol_len;
};
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
static_assert(sizeof(XFBarHeaderFixed) == 60, "header must be 60 bytes");
static_assert(sizeof(RawRecord) == 60, "record must be 60 bytes");
// -----------------------------------------------------------------------------
// days_from_civil — алгоритм Ховарда Хиннанта, без зависимостей от локали/TZ.
// -----------------------------------------------------------------------------
static int64_t days_from_civil(int y, unsigned m, unsigned d) {
    y -= (m <= 2);
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<int64_t>(era) * 146097LL + static_cast<int64_t>(doe) - 719468LL;
}
static bool parse_datetime(const char* s, int64_t& out_sec) {
    int y = 0, mo = 0, d = 0, h = 0, mi = 0;
    if (std::sscanf(s, "%d.%d.%d %d:%d", &y, &mo, &d, &h, &mi) != 5) return false;
    if (y < 1970 || y > 2200 || mo < 1 || mo > 12 || d < 1 || d > 31) return false;
    if (h < 0 || h > 23 || mi < 0 || mi > 59) return false;
    out_sec = days_from_civil(y, static_cast<unsigned>(mo), static_cast<unsigned>(d))
              * 86400LL + h * 3600LL + mi * 60LL;
    return true;
}
static std::vector<std::string> split_csv(const std::string& line) {
    std::vector<std::string> out;
    out.reserve(8);
    size_t start = 0;
    for (size_t i = 0; i <= line.size(); ++i) {
        if (i == line.size() || line[i] == ',') {
            out.push_back(line.substr(start, i - start));
            start = i + 1;
        }
    }
    return out;
}
int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: csv2xfbar <in.csv> <out.bin> <symbol> "
                     "[period_sec=300] [digits=5] [point=0.00001]\n";
        return 1;
    }
    const std::string in_path  = argv[1];
    const std::string out_path = argv[2];
    const std::string symbol   = argv[3];
    const int32_t period_sec   = (argc > 4) ? std::stoi(argv[4]) : 300;
    const int32_t digits       = (argc > 5) ? std::stoi(argv[5]) : 5;
    const double  point        = (argc > 6) ? std::stod(argv[6]) : 0.00001;
    std::ifstream in(in_path);
    if (!in.is_open()) {
        std::cerr << "Cannot open input: " << in_path << "\n";
        return 1;
    }
    std::vector<RawRecord> records;
    records.reserve(600'000);
    std::string line;
    size_t line_no   = 0;
    size_t bad_lines = 0;
    while (std::getline(in, line)) {
        ++line_no;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (line[0] == '<' || line[0] == '#') continue;   // заголовок/комментарий
        auto f = split_csv(line);
        if (f.size() < 7) { ++bad_lines; continue; }
        RawRecord rec{};
        int64_t sec = 0;
        if (!parse_datetime(f[0].c_str(), sec)) { ++bad_lines; continue; }
        try {
            rec.time        = sec;
            rec.open        = std::stod(f[1]);
            rec.high        = std::stod(f[2]);
            rec.low         = std::stod(f[3]);
            rec.close       = std::stod(f[4]);
            rec.tick_volume = std::stoll(f[5]);
            rec.spread      = std::stoi(f[6]);
            rec.real_volume = (f.size() > 7 && !f[7].empty()) ? std::stoll(f[7]) : 0;
        } catch (...) {
            ++bad_lines;
            if (bad_lines <= 10)
                std::cerr << "Bad line " << line_no << ": " << line << "\n";
            continue;
        }
        records.push_back(rec);
    }
    if (records.empty()) {
        std::cerr << "No records parsed\n";
        return 1;
    }
    // Проверка хронологии
    for (size_t i = 1; i < records.size(); ++i) {
        if (records[i].time < records[i - 1].time) {
            std::cerr << "Warning: out-of-order at record " << i
                      << " (t=" << records[i].time << " < "
                      << records[i - 1].time << ")\n";
            break;
        }
    }
    XFBarHeaderFixed hdr{};
    std::memcpy(hdr.magic, "XFBAR001", 8);
    hdr.version        = 1;
    hdr.record_size    = 60;
    hdr.period_seconds = period_sec;
    hdr.digits         = digits;
    hdr.point          = point;
    hdr.bar_count      = static_cast<int64_t>(records.size());
    hdr.first_time     = records.front().time;
    hdr.last_time      = records.back().time;
    hdr.symbol_len     = static_cast<int32_t>(symbol.size());
    std::ofstream out(out_path, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Cannot open output: " << out_path << "\n";
        return 1;
    }
    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    out.write(symbol.data(), static_cast<std::streamsize>(symbol.size()));
    out.write(reinterpret_cast<const char*>(records.data()),
              static_cast<std::streamsize>(records.size() * sizeof(RawRecord)));
    out.close();
    const int64_t file_size =
        static_cast<int64_t>(sizeof(hdr))
      + static_cast<int64_t>(symbol.size())
      + static_cast<int64_t>(records.size() * sizeof(RawRecord));
    std::cout << "Converted: " << in_path << " -> " << out_path << "\n";
    std::cout << "  symbol      : " << symbol      << "\n";
    std::cout << "  period_sec  : " << period_sec  << "\n";
    std::cout << "  digits      : " << digits      << "\n";
    std::cout << "  point       : " << point       << "\n";
    std::cout << "  bars        : " << records.size()  << "\n";
    std::cout << "  first_time  : " << records.front().time << "\n";
    std::cout << "  last_time   : " << records.back().time  << "\n";
    if (bad_lines > 0)
        std::cout << "  bad_lines   : " << bad_lines << " (skipped)\n";
    std::cout << "  file_size   : " << file_size << " bytes\n";
    return 0;
}