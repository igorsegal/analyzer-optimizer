// Smoke: PinbarBuyDetector + PinbarSellDetector.
#include "patterns/PinbarBuyDetector.h"
#include "patterns/PinbarSellDetector.h"
#include <iostream>
#include <iomanip>
using namespace spartak;
static void show_buy(const char* name, const core::Bar& b,
                     const patterns::PinbarBuyDetector& det) {
    auto s = det.detect(b);
    std::cout << name << "\n";
    std::cout << "  O=" << std::setprecision(5) << b.open
              << " H=" << b.high << " L=" << b.low << " C=" << b.close << "\n";
    std::cout << "  detected=" << (s.detected ? "YES" : "NO");
    if (s.detected) {
        std::cout << "  conf=" << std::setprecision(3) << s.confidence
                  << "  trigger=" << s.trigger_price
                  << "  stop=" << s.suggested_stop;
    }
    std::cout << "\n\n";
}
static void show_sell(const char* name, const core::Bar& b,
                      const patterns::PinbarSellDetector& det) {
    auto s = det.detect(b);
    std::cout << name << "\n";
    std::cout << "  O=" << std::setprecision(5) << b.open
              << " H=" << b.high << " L=" << b.low << " C=" << b.close << "\n";
    std::cout << "  detected=" << (s.detected ? "YES" : "NO");
    if (s.detected) {
        std::cout << "  conf=" << std::setprecision(3) << s.confidence
                  << "  trigger=" << s.trigger_price
                  << "  stop=" << s.suggested_stop;
    }
    std::cout << "\n\n";
}
int main() {
    std::cout << "=== Pinbar detectors smoke test ===\n\n";
    patterns::PinbarBuyDetector  buy;
    patterns::PinbarSellDetector sell;
    show_buy("Hammer #1 (ideal BUY)",
             core::Bar{1, 1.0010, 1.0015, 0.9980, 1.0012, 0, 0}, buy);
    show_sell("Shooting Star #1 (ideal SELL)",
              core::Bar{2, 1.0010, 1.0040, 1.0005, 1.0008, 0, 0}, sell);
    show_buy("Normal bullish candle (should NOT be BUY pinbar)",
             core::Bar{3, 1.0000, 1.0025, 0.9995, 1.0020, 0, 0}, buy);
    show_buy("Flat doji (should NOT be BUY pinbar)",
             core::Bar{4, 1.0000, 1.0002, 0.9998, 1.0001, 0, 0}, buy);
    show_buy("Hammer #2 (moderate BUY)",
             core::Bar{5, 1.0020, 1.0025, 0.9975, 1.0022, 0, 0}, buy);
    show_sell("Shooting Star #2 (moderate SELL)",
              core::Bar{6, 1.0020, 1.0065, 1.0018, 1.0022, 0, 0}, sell);
    std::cout << "Done.\n";
    return 0;
}