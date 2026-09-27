#pragma once
#include <array>
#include <cstdint>
namespace spartak::position {
struct VolumeSplitterConfig {
    std::array<double, 3> partial_pcts = { 0.50, 0.25, 0.25 };
    double min_lot  = 0.01;
    double lot_step = 0.01;
};
struct SplitResult {
    bool   ok         = false;
    double close_lot  = 0.0;
    double remain_lot = 0.0;
    bool   full_close = false;
};
class VolumeSplitter50 {
public:
    explicit VolumeSplitter50(VolumeSplitterConfig cfg = {});
    SplitResult split(double initial, double remaining, int level) const noexcept;
    const VolumeSplitterConfig& config() const noexcept { return cfg_; }
private:
    VolumeSplitterConfig cfg_;
};
} // namespace spartak::position