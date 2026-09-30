// Smoke: CandleGeometryCalculator.
#include "data/BarStream.h"
#include "data/DataSanitizer.h"
#include "patterns/CandleGeometryCalculator.h"
#include <iostream>
#include <iomanip>
#include <vector>
using namespace spartak;
int main(int argc, char** argv) {
    std::cout << "=== CandleGeometryCalculator smoke test ===\n\n";
    patterns::CandleGeometryCalculator calc;
    auto show = [&](const char* name, const core::Bar& b) {
        auto g = calc.compute(b);
        std::cout << name << "\n";
        std::cout << "  O=" << std::setprecision(5) << b.open
                  << " H=" << b.high
                  << " L=" << b.low
                  << " C=" << b.close << "\n";
        std::cout << "  body=" << g.body
                  << "  range=" << g.range
                  << "  upper_wick=" << g.upper_wick
                  << "  lower_wick=" << g.lower_wick << "\n";
        std::cout << "  body_ratio=" << g.body_ratio
                  << "  upper_ratio=" << g.upper_ratio
                  << "  lower_ratio=" << g.lower_ratio
                  << "  close_pos=" << g.close_position << "\n";
        std::cout << "  bull=" << (g.is_bull ? "Y" : "N")
                  << "  bear=" << (g.is_bear ? "Y" : "N")
                  << "  doji=" << (g.is_doji ? "Y" : "N") << "\n\n";
    };
    show("Bull candle",
         core::Bar{1, 1.0000, 1.0025, 0.9990, 1.0020, 0, 0});
    show("Bear candle",
         core::Bar{2, 1.0020, 1.0025, 0.9990, 1.0000, 0, 0});
    show("Doji",
         core::Bar{3, 1.0000, 1.0020, 0.9980, 1.00005, 0, 0});
    show("Hammer-like",
         core::Bar{4, 1.0010, 1.0012, 0.9975, 1.0010, 0, 0});
    std::vector<core::Bar> test_bars;
    for (int i = 0; i < 20; ++i) {
        double base = 1.0000 + i * 0.0001;
        test_bars.push_back({i, base, base + 0.0020, base - 0.0020, base, 0, 0});
    }
    double a = calc.atr(test_bars, 14);
    double ar = calc.average_range(test_bars, 14);
    std::cout << "Synthetic ATR(14)         : " << std::setprecision(6) << a  << "\n";
    std::cout << "Synthetic avg_range(14)   : " << ar << "\n";
    std::cout << "Expected range per bar    : 0.004000\n";
    std::cout << "ATR >= range (TR includes gaps) : "
              << (a >= ar - 1e-9 ? "OK" : "FAIL") << "\n\n";
    const std::string path = (argc > 1)
        ? argv[1]
        : "D:/AHexaTrader/1DataFiles/raw/EURUSD/EURUSD_M5_real.bin";
    data::BarStream stream(path);
    if (!stream.is_ok()) {
        std::cout << "Stream open FAILED for: " << path << "\n";
        return 1;
    }
    data::DataSanitizer sanitizer;
    (void)sanitizer.run(stream);
    constexpr int N = 100;
    std::vector<core::Bar> real_bars;
    real_bars.reserve(N);
    core::Bar b;
    while (real_bars.size() < N && stream.next(b)) real_bars.push_back(b);
    const double real_atr = calc.atr(real_bars, 14);
    const double real_ar  = calc.average_range(real_bars, 14);
    std::cout << "--- Real M5 (first 100 bars) ---\n";
    std::cout << "Real ATR(14)              : " << std::setprecision(6) << real_atr << "\n";
    std::cout << "Real avg_range(14)        : " << real_ar << "\n";
    std::cout << "Range sum check           : "
              << (real_atr >= real_ar - 1e-9 ? "OK" : "FAIL") << "\n\n";
    int bad = 0;
    for (const auto& bb : real_bars) {
        auto g = calc.compute(bb);
        if (g.range > 0.0) {
            const double sum = g.body_ratio + g.upper_ratio + g.lower_ratio;
            if (std::fabs(sum - 1.0) > 1e-9) ++bad;
            if (g.close_position < 0.0 || g.close_position > 1.0) ++bad;
        }
    }
    std::cout << "Consistency (body+wiches=1, close_pos in [0,1]): "
              << (bad == 0 ? "OK" : "FAIL") << " (bad=" << bad << ")\n";
    std::cout << "\nDone.\n";
    return 0;
}