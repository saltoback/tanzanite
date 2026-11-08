// Tanzanite — Show Trajectory
// Every frame, an invisible copy of the player is simulated a short time
// ahead twice: once holding the button (green) and once releasing it (red).
#pragma once

#include <Geode/Geode.hpp>

namespace tanzanite::traj {
    bool simulating();
    void onSimDeath();
    void onLevelInit(geode::prelude::PlayLayer* pl);
    void onQuit();
    void update(geode::prelude::PlayLayer* pl);
}
