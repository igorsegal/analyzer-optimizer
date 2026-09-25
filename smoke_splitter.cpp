// Smoke: VolumeSplitter50.
#include "position/VolumeSplitter50.h"
#include <iostream>
#include <iomanip>
#include <cmath>
using namespace spartak;
static bool approx(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) < eps;
}
int main() {
    std::cout << "=== VolumeSplitter50 smoke test ===\n\n";
    std::cout << std::fixed << std::setprecision(2);
    // ========== Стандартный шаг 0.01, partial 50% ==========
    position::VolumeSplitterConfig cfg;
    cfg.partial_pct = 0.50;
    cfg.min_lot     = 0.01;
    cfg.lot_step    = 0.01;
    position::VolumeSplitter50 s(cfg);
    // --- T1: 1.00 лот -> 0.50 / 0.50 ---
    {
        auto r = s.split(1.00, 1.00);
        std::cout << "T1 split(1.00, 1.00)  -> "
                  << "close=" << r.close_lot << "  remain=" << r.remain_lot
                  << "  ok=" << (r.ok ? "Y" : "N")
                  << "  full_close=" << (r.full_close ? "Y" : "N")
                  << "  (expect 0.50/0.50)  "
                  << (r.ok && !r.full_close && approx(r.close_lot, 0.50) && approx(r.remain_lot, 0.50) ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T2: 0.10 лот -> 0.05 / 0.05 ---
    {
        auto r = s.split(0.10, 0.10);
        std::cout << "T2 split(0.10, 0.10)  -> "
                  << "close=" << r.close_lot << "  remain=" << r.remain_lot
                  << "  (expect 0.05/0.05)  "
                  << (r.ok && !r.full_close && approx(r.close_lot, 0.05) && approx(r.remain_lot, 0.05) ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T3: 0.05 лот -> 0.02 (floor от 0.025) / 0.03 ---
    {
        auto r = s.split(0.05, 0.05);
        std::cout << "T3 split(0.05, 0.05)  -> "
                  << "close=" << r.close_lot << "  remain=" << r.remain_lot
                  << "  (expect 0.02/0.03)  "
                  << (r.ok && !r.full_close && approx(r.close_lot, 0.02) && approx(r.remain_lot, 0.03) ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T4: 0.03 лот -> 0.01 / 0.02 ---
    {
        auto r = s.split(0.03, 0.03);
        std::cout << "T4 split(0.03, 0.03)  -> "
                  << "close=" << r.close_lot << "  remain=" << r.remain_lot
                  << "  (expect 0.01/0.02)  "
                  << (r.ok && !r.full_close && approx(r.close_lot, 0.01) && approx(r.remain_lot, 0.02) ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T5: 0.02 лот — split невозможен, full_close ---
    {
        // 0.02 / 0.50 = 0.01 close, остаток 0.01 = min_lot -> валидный split
        auto r = s.split(0.02, 0.02);
        std::cout << "T5 split(0.02, 0.02)  -> "
                  << "close=" << r.close_lot << "  remain=" << r.remain_lot
                  << "  full_close=" << (r.full_close ? "Y" : "N")
                  << "  (expect 0.01/0.01)  "
                  << (r.ok && !r.full_close && approx(r.close_lot, 0.01) && approx(r.remain_lot, 0.01) ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T6: 0.01 лот — тоже full_close ---
    {
        auto r = s.split(0.01, 0.01);
        std::cout << "T6 split(0.01, 0.01)  -> "
                  << "close=" << r.close_lot << "  remain=" << r.remain_lot
                  << "  full_close=" << (r.full_close ? "Y" : "N")
                  << "  (expect full 0.01/0)  "
                  << (r.ok && r.full_close && approx(r.close_lot, 0.01) && approx(r.remain_lot, 0.0) ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T7: 0.50 лот -> 0.25 / 0.25 ---
    {
        auto r = s.split(0.50, 0.50);
        std::cout << "T7 split(0.50, 0.50)  -> "
                  << "close=" << r.close_lot << "  remain=" << r.remain_lot
                  << "  (expect 0.25/0.25)  "
                  << (r.ok && !r.full_close && approx(r.close_lot, 0.25) && approx(r.remain_lot, 0.25) ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T8: partial 30% на 1.00 лот -> 0.30 / 0.70 ---
    {
        position::VolumeSplitterConfig cfg30;
        cfg30.partial_pct = 0.30;
        cfg30.min_lot     = 0.01;
        cfg30.lot_step    = 0.01;
        position::VolumeSplitter50 s30(cfg30);
        auto r = s30.split(1.00, 1.00);
        std::cout << "T8 partial=0.30       -> "
                  << "close=" << r.close_lot << "  remain=" << r.remain_lot
                  << "  (expect 0.30/0.70)  "
                  << (r.ok && !r.full_close && approx(r.close_lot, 0.30) && approx(r.remain_lot, 0.70) ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T9: partial 25% на 1.00 -> 0.25 / 0.75 ---
    {
        position::VolumeSplitterConfig cfg25;
        cfg25.partial_pct = 0.25;
        cfg25.min_lot     = 0.01;
        cfg25.lot_step    = 0.01;
        position::VolumeSplitter50 s25(cfg25);
        auto r = s25.split(1.00, 1.00);
        std::cout << "T9 partial=0.25       -> "
                  << "close=" << r.close_lot << "  remain=" << r.remain_lot
                  << "  (expect 0.25/0.75)  "
                  << (r.ok && !r.full_close && approx(r.close_lot, 0.25) && approx(r.remain_lot, 0.75) ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T10: split(1.00, 0.50) — ситуация после первого partial ---
    // initial=1.00 (для расчета 50%), remaining=0.50 (текущий)
    // close = 0.50 (из initial), but remaining only 0.50 -> wait
    // Здесь split не имеет смысла, так как partial применяется один раз.
    // Проверим corner-case: initial=1.00, remaining=0.30
    // close_raw = 0.50, remain_raw = 0.30-0.50 < 0 -> full_close
    {
        auto r = s.split(1.00, 0.30);
        std::cout << "T10 split(1.00, 0.30) -> "
                  << "close=" << r.close_lot << "  remain=" << r.remain_lot
                  << "  full_close=" << (r.full_close ? "Y" : "N")
                  << "  (expect full 0.30/0)  "
                  << (r.ok && r.full_close && approx(r.close_lot, 0.30) && approx(r.remain_lot, 0.0) ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T11: нулевые входы ---
    {
        auto r0 = s.split(0.0, 0.0);
        auto r1 = s.split(1.0, 0.0);
        std::cout << "T11 zero inputs       -> "
                  << "split(0,0).ok=" << (r0.ok ? "Y" : "N")
                  << "  split(1,0).ok=" << (r1.ok ? "Y" : "N")
                  << "  (expect both N)  "
                  << (!r0.ok && !r1.ok ? "OK" : "FAIL")
                  << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}