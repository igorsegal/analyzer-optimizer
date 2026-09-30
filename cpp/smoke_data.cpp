// Временный smoke-тест для слоя data/
// Проверяет, что BarStream выдаёт бары в обоих режимах.
#include "data/BarStream.h"
#include "core/Config.h"
#include <iostream>
#include <iomanip>
int main() {
    using namespace spartak;
    std::cout << std::fixed << std::setprecision(5);
    std::cout << "=== BarStream smoke test ===\n\n";
    // --- Синтетический режим ---
    data::BarStream::SyntheticParams sp;
    sp.start_price   = 1.10000;
    sp.point         = 0.00001;
    sp.spread_points = 12;
    sp.max_bars      = 10;
    sp.seed          = 1337;
    data::BarStream stream(sp);
    std::cout << "mode      = " << (stream.mode() == data::StreamMode::Synthetic ? "Synthetic" : "File") << "\n";
    std::cout << "total     = " << stream.total_bars() << "\n";
    std::cout << "is_ok     = " << (stream.is_ok() ? "true" : "false") << "\n\n";
    core::Bar bar;
    int n = 0;
    while (stream.next(bar)) {
        ++n;
        std::cout << "Bar #" << std::setw(2) << n
                  << "  t=" << bar.timestamp
                  << "  O=" << bar.open
                  << "  H=" << bar.high
                  << "  L=" << bar.low
                  << "  C=" << bar.close
                  << "  vol=" << bar.tick_volume
                  << "  spr=" << bar.spread
                  << "\n";
    }
    std::cout << "\nbars_read = " << stream.bars_read() << "\n";
    std::cout << "Done.\n";
    return 0;
}