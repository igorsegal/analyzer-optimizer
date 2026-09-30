// Smoke: TrendBiasEvaluator на синтетике + на реальных данных.
// Проверяет все 5 сценариев логики.
#include "context/TrendBiasEvaluator.h"
#include <iostream>
#include <vector>
using namespace spartak;
using context::Extremum;
using context::TrendBias;
static std::vector<Extremum> make_highs(std::initializer_list<double> prices) {
    std::vector<Extremum> v;
    std::size_t i = 1;
    for (double p : prices) {
        v.push_back({Extremum::Kind::High, i * 2, 0, p});
        ++i;
    }
    return v;
}
static std::vector<Extremum> make_lows(std::initializer_list<double> prices) {
    std::vector<Extremum> v;
    std::size_t i = 1;
    for (double p : prices) {
        v.push_back({Extremum::Kind::Low, i * 2 + 1, 0, p});
        ++i;
    }
    return v;
}
static const char* dir_name(core::TrendDirection d) {
    switch (d) {
        case core::TrendDirection::Bullish: return "Bullish";
        case core::TrendDirection::Bearish: return "Bearish";
        default:                            return "Undefined";
    }
}
int main() {
    context::TrendBiasEvaluator eval(3);
    std::cout << "=== TrendBiasEvaluator smoke test ===\n\n";
    // --- Сценарий 1: оба Bullish ---
    {
        auto dh = make_highs({1.10, 1.15, 1.20});
        auto dl = make_lows ({1.05, 1.10, 1.15});
        auto hh = make_highs({1.11, 1.16, 1.21});
        auto hl = make_lows ({1.06, 1.11, 1.16});
        auto b  = eval.evaluate(dh, dl, hh, hl);
        std::cout << "1) Daily=H1=Bullish    -> " << dir_name(b.direction)
                  << "  both_agree=" << (b.both_agree ? "Y" : "N") << "  ";
        bool ok = (b.direction == core::TrendDirection::Bullish) && b.both_agree;
        std::cout << (ok ? "OK" : "FAIL") << "\n";
    }
    // --- Сценарий 2: оба Bearish ---
    {
        auto dh = make_highs({1.20, 1.15, 1.10});
        auto dl = make_lows ({1.15, 1.10, 1.05});
        auto hh = make_highs({1.21, 1.16, 1.11});
        auto hl = make_lows ({1.16, 1.11, 1.06});
        auto b  = eval.evaluate(dh, dl, hh, hl);
        std::cout << "2) Daily=H1=Bearish    -> " << dir_name(b.direction)
                  << "  both_agree=" << (b.both_agree ? "Y" : "N") << "  ";
        bool ok = (b.direction == core::TrendDirection::Bearish) && b.both_agree;
        std::cout << (ok ? "OK" : "FAIL") << "\n";
    }
    // --- Сценарий 3: Daily определён, H1 Undefined ---
    {
        auto dh = make_highs({1.10, 1.15, 1.20});
        auto dl = make_lows ({1.05, 1.10, 1.15});
        // H1: смешанный (не HH-HL, не LL-LH) -> Undefined
        auto hh = make_highs({1.11, 1.16, 1.13});
        auto hl = make_lows ({1.06, 1.11, 1.09});
        auto b  = eval.evaluate(dh, dl, hh, hl);
        std::cout << "3) Daily=Bull, H1=Undef -> " << dir_name(b.direction)
                  << "  fallback=" << (b.fallback ? "Y" : "N") << "  ";
        bool ok = (b.direction == core::TrendDirection::Bullish) && b.fallback;
        std::cout << (ok ? "OK" : "FAIL") << "\n";
    }
    // --- Сценарий 4: конфликт Daily=Bull, H1=Bear ---
    {
        auto dh = make_highs({1.10, 1.15, 1.20});
        auto dl = make_lows ({1.05, 1.10, 1.15});
        auto hh = make_highs({1.21, 1.16, 1.11});
        auto hl = make_lows ({1.16, 1.11, 1.06});
        auto b  = eval.evaluate(dh, dl, hh, hl);
        std::cout << "4) Daily=Bull,H1=Bear  -> " << dir_name(b.direction)
                  << "  conflict=" << (b.conflict ? "Y" : "N") << "  ";
        bool ok = (b.direction == core::TrendDirection::Undefined) && b.conflict;
        std::cout << (ok ? "OK" : "FAIL") << "\n";
    }
    // --- Сценарий 5: оба Undefined ---
    {
        auto dh = make_highs({1.10, 1.15, 1.13});
        auto dl = make_lows ({1.05, 1.10, 1.08});
        auto hh = make_highs({1.20, 1.15, 1.17});
        auto hl = make_lows ({1.15, 1.10, 1.12});
        auto b  = eval.evaluate(dh, dl, hh, hl);
        std::cout << "5) Both undefined      -> " << dir_name(b.direction) << "  ";
        bool ok = (b.direction == core::TrendDirection::Undefined)
               && !b.conflict && !b.both_agree && !b.fallback;
        std::cout << (ok ? "OK" : "FAIL") << "\n";
    }
    std::cout << "\nDone.\n";
    return 0;
}