#include "context/HighExtractor.h"
namespace spartak::context {
std::vector<Extremum>
HighExtractor::extract(const std::vector<Extremum>& extrema) {
    std::vector<Extremum> out;
    out.reserve(extrema.size() / 2 + 1);
    for (const auto& e : extrema) {
        if (e.kind == Extremum::Kind::High) {
            out.push_back(e);
        }
    }
    return out;
}
} // namespace spartak::context