#ifndef AHEXA_DATALOADER_H
#define AHEXA_DATALOADER_H

#include "common.h"
#include <vector>
#include <string>

namespace AHexa {

class DataLoader {
public:
    // Сканирует папку, возвращает список инструментов
    std::vector<InstrumentInfo> scan(const std::string& rootDir);

    // Загружает бары M5 из файла
    std::vector<Bar> loadBars(const InstrumentInfo& info);

private:
    bool validateHeader(const std::string& filePath);
};

} // namespace AHexa

#endif