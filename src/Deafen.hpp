// Tanzanite — Auto Deafen: presses your Discord deafen shortcut at a chosen %
// (Windows only: a phone can't send keyboard shortcuts to Discord)
#pragma once

#include <Geode/Geode.hpp>
#include <string>

class PlayLayer;

namespace tanzanite::deafen {
    // The menu is waiting for the key of the shortcut
    inline bool capturing = false;
    // Goes up every time the shortcut changes (the menu redraws)
    inline int version = 0;

    // Is Auto Deafen available on this platform?
    bool supported();
    // "Ctrl + Shift + D"
    std::string shortcutName();
    // Presses the shortcut once (Discord toggles deafen)
    void press();

    // Called with a key press while capturing; true when it was used
    bool capture(int vk);

    void onUpdate(PlayLayer* pl);   // every frame in a level
    void onReset();                 // new attempt
    void onQuit();                  // leaving the level
}
