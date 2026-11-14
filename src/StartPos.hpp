// Tanzanite — StartPos Switcher: jump between the start positions of a level
// (Q / D on PC, or the arrows at the bottom of the screen)
#pragma once

#include <Geode/Geode.hpp>

namespace tanzanite::startpos {
    // -1: previous start position, +1: next one (wraps around, 0 = level start)
    void switchBy(int dir);
    // Is a screen point on the switcher's arrows? (clicks there must not jump)
    bool blocksPoint(geode::prelude::CCPoint world);
}
