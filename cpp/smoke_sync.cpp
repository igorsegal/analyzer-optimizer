// Smoke: PositionStateSynchronizer.
#include "position/PositionStateSynchronizer.h"
#include <iostream>
#include <iomanip>
#include <cmath>
using namespace spartak;
static bool approx(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) < eps;
}
int main() {
    std::cout << "=== PositionStateSynchronizer smoke test ===\n\n";
    std::cout << std::fixed << std::setprecision(2);
    // --- T1: markOpened ---
    {
        position::PositionState s;
        position::PositionStateSynchronizer::markOpened(
            s, /*entry*/1.1000, /*vol*/1.0, /*sl*/1.0980,
            /*tp1*/1.1030, /*tp2*/1.1060, /*spread*/10, /*time*/1000);
        std::cout << "T1 markOpened         : "
                  << "vol=" << s.initial_volume << "/" << s.remaining_volume
                  << "  sl=" << s.stop_loss
                  << "  tp1_hit=" << (s.tp1_hit ? "Y" : "N")
                  << "  closed=" << (s.closed ? "Y" : "N")
                  << "  (expect 1.00/1.00/sl=1.09800/N/N)  "
                  << (approx(s.initial_volume, 1.0) && approx(s.remaining_volume, 1.0)
                      && approx(s.stop_loss, 1.0980) && !s.tp1_hit && !s.closed
                      ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T2: partial close 50% ---
    {
        position::PositionState s;
        s.initial_volume = 1.0;
        s.remaining_volume = 1.0;
        position::PositionStateSynchronizer::markPartialClose(
            s, /*closed_vol*/0.5, /*price*/1.1030,
            /*gross*/150.0, /*comm*/1.25, /*swap*/0.5, /*tp1*/true);
        std::cout << "T2 partialClose 50%   : "
                  << "remain=" << s.remaining_volume
                  << "  tp1_hit=" << (s.tp1_hit ? "Y" : "N")
                  << "  realized=" << s.realized_pnl
                  << "  comm=" << s.total_commission
                  << "  (expect 0.50/Y/150.00/1.25)  "
                  << (approx(s.remaining_volume, 0.5) && s.tp1_hit
                      && approx(s.realized_pnl, 150.0) && approx(s.total_commission, 1.25)
                      ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T3: partial close с нулевым остатком -> closed ---
    {
        position::PositionState s;
        s.initial_volume = 1.0;
        s.remaining_volume = 0.5;
        position::PositionStateSynchronizer::markPartialClose(
            s, 0.5, 1.1030, 150.0, 1.25, 0.0, false);
        std::cout << "T3 partialClose -> 0  : "
                  << "remain=" << s.remaining_volume
                  << "  closed=" << (s.closed ? "Y" : "N")
                  << "  (expect 0.00/Y)  "
                  << (approx(s.remaining_volume, 0.0) && s.closed ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T4: markStopMoved (trail без BE) ---
    {
        position::PositionState s;
        s.stop_loss = 1.0980;
        s.be_moved = false;
        position::PositionStateSynchronizer::markStopMoved(s, 1.1000, false);
        std::cout << "T4 stopMoved (trail)  : "
                  << "sl=" << s.stop_loss
                  << "  be_moved=" << (s.be_moved ? "Y" : "N")
                  << "  (expect 1.10000/N)  "
                  << (approx(s.stop_loss, 1.1000) && !s.be_moved ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T5: markStopMoved с be_done=true ---
    {
        position::PositionState s;
        s.stop_loss = 1.0980;
        position::PositionStateSynchronizer::markStopMoved(s, 1.1001, true);
        std::cout << "T5 stopMoved (BE)     : "
                  << "sl=" << s.stop_loss
                  << "  be_moved=" << (s.be_moved ? "Y" : "N")
                  << "  (expect 1.10010/Y)  "
                  << (approx(s.stop_loss, 1.1001) && s.be_moved ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T6: accrueSwap ---
    {
        position::PositionState s;
        position::PositionStateSynchronizer::accrueSwap(s, -7.0, 19723);
        std::cout << "T6 accrueSwap(-7.00)  : "
                  << "total_swap=" << s.total_swap
                  << "  last_day=" << s.last_swap_day
                  << "  (expect -7.00/19723)  "
                  << (approx(s.total_swap, -7.0) && s.last_swap_day == 19723 ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T7: несколько accruals суммируются ---
    {
        position::PositionState s;
        position::PositionStateSynchronizer::accrueSwap(s, -7.0, 19723);
        position::PositionStateSynchronizer::accrueSwap(s, -7.0, 19724);
        position::PositionStateSynchronizer::accrueSwap(s, -7.0, 19725);
        std::cout << "T7 3 nights accrued   : "
                  << "total_swap=" << s.total_swap
                  << "  last_day=" << s.last_swap_day
                  << "  (expect -21.00/19725)  "
                  << (approx(s.total_swap, -21.0) && s.last_swap_day == 19725 ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T8: markFullClose ---
    {
        position::PositionState s;
        s.remaining_volume = 0.5;
        position::PositionStateSynchronizer::markFullClose(
            s, 1.1060, 150.0, 1.25, 7.0, 5000, "tp2");
        std::cout << "T8 fullClose          : "
                  << "remain=" << s.remaining_volume
                  << "  closed=" << (s.closed ? "Y" : "N")
                  << "  reason=" << s.close_reason
                  << "  realized=" << s.realized_pnl
                  << "  (expect 0.00/Y/tp2/150.00)  "
                  << (approx(s.remaining_volume, 0.0) && s.closed
                      && s.close_reason == "tp2" && approx(s.realized_pnl, 150.0)
                      ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T9: netPnl ---
    {
        position::PositionState s;
        s.realized_pnl = 300.0;
        s.total_commission = 2.5;
        s.total_swap = -14.0;
        const double np = position::PositionStateSynchronizer::netPnl(s);
        // 300 - 2.5 + (-14) = 283.5
        std::cout << "T9 netPnl             : " << np
                  << "  (expect 283.50)  "
                  << (approx(np, 283.5) ? "OK" : "FAIL")
                  << "\n";
    }
    // --- T10: is_active после partial ---
    {
        position::PositionState s;
        s.remaining_volume = 0.5;
        s.closed = false;
        const bool a1 = s.is_active();
        s.remaining_volume = 0.0;
        s.closed = true;
        const bool a2 = s.is_active();
        std::cout << "T10 is_active         : "
                  << "partial=" << (a1 ? "Y" : "N")
                  << "  closed=" << (a2 ? "Y" : "N")
                  << "  (expect Y/N)  "
                  << (a1 && !a2 ? "OK" : "FAIL")
                  << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}