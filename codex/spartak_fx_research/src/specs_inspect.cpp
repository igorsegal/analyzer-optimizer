#include "spartak/specs.hpp"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: spartak_specs_inspect <symbol_specs_normalized.csv>\n";
        return 2;
    }
    try {
        const auto book = spartak::specs::load_normalized_csv(argv[1]);
        std::size_t spot_fx = 0;
        std::size_t metals = 0;
        std::size_t crypto = 0;
        std::size_t missing_commission = 0;
        std::size_t missing_swap_or_funding = 0;
        for (const auto& row : book.rows()) {
            switch (row.asset_class) {
            case spartak::specs::AssetClass::spot_fx:
                ++spot_fx;
                break;
            case spartak::specs::AssetClass::metal:
                ++metals;
                break;
            case spartak::specs::AssetClass::crypto:
                ++crypto;
                break;
            }
            missing_commission += row.commission_per_lot_per_side ? 0U : 1U;
            missing_swap_or_funding += row.swap_or_funding ? 0U : 1U;
        }
        std::cout
            << "symbols,spot_fx,metals,crypto,missing_commission,missing_swap_or_funding\n"
            << book.rows().size() << ','
            << spot_fx << ','
            << metals << ','
            << crypto << ','
            << missing_commission << ','
            << missing_swap_or_funding << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "spec inspection failed: " << error.what() << '\n';
        return 1;
    }
}
