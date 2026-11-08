// Tanzanite — macro mode: the in-level macro interface
#pragma once

#include <Geode/Geode.hpp>

namespace tanzanite::hud {
    // Turn macro mode on/off (from the menu or the interface itself)
    void enterMacroMode();
    void exitMacroMode();

    // Level events, called from the PlayLayer hooks
    void onLevelInit(geode::prelude::PlayLayer* pl);
    void onQuit();

    // True when a screen point is on the interface (clicks there must not jump)
    bool blocksPoint(geode::prelude::CCPoint world);
}
