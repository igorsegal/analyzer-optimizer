// =============================================================================
//  STPatterns :: st_emulator_main.cpp
//  Entry point. Reads XFBAR file, prints first bars.
// =============================================================================
#include "data/BarStream.h"
#include <cstdio>
#include <cstring>
#include <string>

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: st_emulator <file.bin>\n");
        return 1;
    }
    const std::string path = argv[1];

    spartak::data::BarStream stream(path, 4096);
    if (!stream.is_ok()) {
        std::fprintf(stderr, "cannot open: %s\n", path.c_str());
        return 1;
    }

    std::printf("file: %s\n", path.c_str());

    spartak::core::Bar b;
    int n = 0;
    while (stream.next(b) && n < 5) {
        std::printf("bar %d: t=%lld o=%.5f h=%.5f l=%.5f c=%.5f spread=%d\n",
                    n, (long long)b.timestamp,
                    b.open, b.high, b.low, b.close, b.spread);
        ++n;
    }
    std::printf("read %d bars, ok\n", n);
    return 0;
}