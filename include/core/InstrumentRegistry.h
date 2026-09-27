// =============================================================================
//  SPARTAK :: core/InstrumentRegistry.h
//  Реестр инструментов + автоопределение параметров.
// =============================================================================
#pragma once
#include "core/InstrumentSpec.h"
#include <string>
#include <vector>
#include <optional>
namespace spartak::core {
class InstrumentRegistry {
public:
    // Точное совпадение по символу (регистр игнорируется).
    [[nodiscard]] static std::optional<InstrumentSpec>
    lookup(const std::string& symbol);
    // Если точного совпадения нет — угадываем по префиксу/суффиксу.
    // Например, "EURJPY" -> ForexCross, "XAUUSD" -> Metal, "BWXT" -> Stock.
    [[nodiscard]] static InstrumentSpec
    infer(const std::string& symbol);
    // lookup + fallback на infer.
    [[nodiscard]] static InstrumentSpec resolve(const std::string& symbol);
    // Зарегистрировать/перезаписать параметры инструмента.
    static void registerSpec(const InstrumentSpec& spec);
    // Список всех зарегистрированных символов.
    [[nodiscard]] static std::vector<std::string> list();
private:
    static std::vector<InstrumentSpec>& storage();
    static bool isForexMajor(const std::string& s);
    static bool isForexCross(const std::string& s);
    static bool isMetal(const std::string& s);
    static bool isIndex(const std::string& s);
    static bool isCrypto(const std::string& s);
};
} // namespace spartak::core