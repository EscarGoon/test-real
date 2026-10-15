#pragma once
#include "../core/Engine.hpp"

#include <string>
#include <vector>

namespace inspo {

struct GenSettings {
    std::string theme = "random";
    std::string harmony = "auto";
    std::string layout = "mix";   // "mix" or one structure type name
    int count = 3;                // structures per batch
    int detail = 2;               // 1..3
    int motion = 2;               // 0 = static, 1 = ambient, 2 = full
    bool background = true;
    bool parallax = true;
    bool entrance = true;
    bool setLevelColors = true;   // BG / ground / line
    bool startAtLevelBegin = true;
    int groupStart = 500;
    int colorStart = 970;         // uses ROLE_COUNT channels from here
    int solidId = gd::obj::DEFAULT_SOLID;
    double centerX = 600;         // camera center in level-string coords (ground at y = 0)
    double centerY = 150;
    uint32_t seed = 1;            // structure layout + shapes
    uint32_t paletteSeed = 1;     // colors only
};

struct GenResult {
    std::string levelString;
    size_t objects = 0, triggers = 0;
    int firstGroup = 0, nextGroup = 0;
    Palette palette;
    std::vector<std::string> placed;
    double x0 = 0, x1 = 0;
};

std::vector<std::string> const& structureTypes();
GenResult generate(GenSettings const& s);

} // namespace inspo
