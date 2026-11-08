// Tanzanite — macro bot (record / play) and frame stepper
//
// How it stays in sync:
//  - Inputs are stored by physics tick (240 per second of level time), never
//    by real time, so the speedhack or the FPS have no effect on them.
//  - Every input also stores the exact player state (position, rotation,
//    velocities). On playback that state is restored right before the input,
//    so tiny float differences can never pile up ("position fix").
//  - At practice checkpoints, a full snapshot of both players is taken and
//    restored on respawn (the game's own checkpoints lose precision).
#pragma once

#include <Geode/Geode.hpp>
#include <filesystem>
#include <string>
#include <vector>

namespace tanzanite::bot {
    using namespace geode::prelude;

    enum class Mode { Off = 0, Record, Play };

    struct Input {
        int frame;
        int button;     // 1 = jump, 2 = left, 3 = right
        bool down;
        bool player1;   // raw "isPlayer1" argument of handleButton
    };

    struct PlayerState {
        float x = 0.f, y = 0.f, rotation = 0.f;
        double yVel = 0.0, xVel = 0.0;
    };

    struct Fix {
        int frame;
        PlayerState p1, p2;
    };

    struct Macro {
        std::string name;
        std::string levelKey;
        std::vector<Input> inputs;
        std::vector<Fix> fixes;

        void clear() { inputs.clear(); fixes.clear(); }
        int lastFrame() const { return inputs.empty() ? 0 : inputs.back().frame; }
    };

    inline Mode mode = Mode::Off;
    inline Macro macro;
    inline bool positionFix = true;
    inline bool stepper = false;
    inline int stepRequests = 0;

    // Playback progress (for the labels)
    inline size_t playIndex = 0;

    void setMode(Mode m);
    void setStepper(bool on);

    std::filesystem::path macroDir();
    std::vector<std::filesystem::path> listMacros();
    bool saveMacro(std::string const& name);
    bool loadMacro(std::filesystem::path const& path);
    std::string levelKey(GJGameLevel* level);
    std::string macroInfo();

    void loadSettings();
    void saveSettings();

    // Level events, called from the PlayLayer hooks
    void onLevelInit(PlayLayer* pl);
    void onReset(PlayLayer* pl);
    void onLevelComplete(PlayLayer* pl);
    void onQuit();
}
