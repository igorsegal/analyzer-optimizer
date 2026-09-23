#pragma once

#include "structures.h"
#include <vector>
#include <string>

// ============================================================================
// ЗАГРУЗЧИК ДАННЫХ (XFBAR FORMAT)
// Соответствует формату .bin файлов (60 байт на бар + заголовок)
// ============================================================================
class DataLoader {
public:
    // Загрузка конкретного .bin файла
    // Возвращает вектор баров (пустой, если ошибка)
    static std::vector<XFBAR> loadBin(const std::string& filepath);
    
    // Загрузка по символу и Таймфрейму
    // Путь формируется автоматически: folder\SYMBOL\SYMBOL_TF.bin
    static std::vector<XFBAR> loadSymbol(const std::string& folder, 
                                         const std::string& symbol, 
                                         const std::string& timeframe);
    
    // Получение дневных баров для контекста (GRAAL: Daily Context)
    // Если текущий ТФ != D1, загружает D1 данные для того же символа
    static std::vector<XFBAR> loadDailyContext(const std::string& folder,
                                               const std::string& symbol,
                                               const std::string& currentTf);

private:
    // Внутренняя функция пропуска заголовка XFBAR
    static size_t skipHeader(std::ifstream& file);
};