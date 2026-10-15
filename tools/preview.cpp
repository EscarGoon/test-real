// Standalone generator for testing outside the game.
// Usage: preview <seed> [theme] [layout] [count] [detail] > out.txt
// Prints the GD object string; tools/render.py turns it into a PNG.
#include "../src/gen/Generator.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    inspo::GenSettings s;
    s.seed = argc > 1 ? (uint32_t)std::strtoul(argv[1], nullptr, 10) : 1;
    s.paletteSeed = s.seed;
    if (argc > 2) s.theme = argv[2];
    if (argc > 3) s.layout = argv[3];
    if (argc > 4) s.count = std::atoi(argv[4]);
    if (argc > 5) s.detail = std::atoi(argv[5]);
    s.centerX = 900;
    s.centerY = 150;
    auto r = inspo::generate(s);
    std::cout << r.levelString << "\n";
    std::fprintf(stderr, "palette=%s objects=%zu triggers=%zu groups=%d..%d placed=", r.palette.name.c_str(),
                 r.objects, r.triggers, r.firstGroup, r.nextGroup - 1);
    for (auto const& p : r.placed) std::fprintf(stderr, "%s ", p.c_str());
    std::fprintf(stderr, "\n");
}
