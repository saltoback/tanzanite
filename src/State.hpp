// Tanzanite — shared state (saved options + current attempt stats)
#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <vector>

namespace tanzanite {
    using namespace geode::prelude;

    // Theme colors
    inline const ccColor3B PURPLE = {150, 110, 255};
    inline const ccColor4F PURPLE4 = {0.59f, 0.43f, 1.f, 1.f};
    inline const ccColor4F PANEL4 = {0.04f, 0.02f, 0.09f, 0.78f};
    inline const ccColor4F SIDEBAR4 = {0.10f, 0.06f, 0.22f, 0.70f};

    // Screen corners for the labels
    enum Corner { TopLeft = 0, TopRight, BottomLeft, BottomRight };

    struct State {
        // Player
        bool noclip = false;
        bool speedhack = false;
        float speed = 1.0f;
        bool showHitboxes = false;
        bool showTrajectory = false;
        // Level
        bool safeMode = false;
        bool autoSafe = true;
        bool accuratePct = false;
        // Labels
        bool lblCheat = true;
        bool lblFps = false;
        bool lblCps = false;
        bool lblNoclipAcc = false;
        bool lblNoclipDeaths = false;
        bool lblAttempts = false;
        bool lblJumps = false;
        bool lblTime = false;
        bool lblClock = false;
        bool lblMessage = false;
        bool lblBestRun = false;
        bool bestRunAccurate = true;    // Best Run with 2 decimals
        bool speedhackMusic = true;     // music follows the speedhack
        bool noParticles = false;       // hide the player's particles
        bool noIconEffects = false;     // hide trails, wave trail, ship fire, ghost trail
        std::vector<int> labelOrder;    // order of the labels on screen (LabelId)
        std::string message = "Tanzanite";
        int corner = TopLeft;
        float labelScale = 1.0f;
        int labelOpacity = 180;
    };

    inline State s;

    // Labels, in their default order
    enum LabelId {
        LblCheat = 0, LblFps, LblCps, LblNoclipAcc, LblNoclipDeaths, LblAttempts,
        LblJumps, LblTime, LblClock, LblBestRun, LblMessage, LABEL_COUNT
    };

    inline char const* labelName(int id) {
        static char const* names[LABEL_COUNT] = {
            "Cheat Indicator", "FPS", "CPS", "Noclip Accuracy", "Noclip Deaths", "Attempts",
            "Jumps", "Level Time", "Clock", "Best Run", "Custom Message",
        };
        return (id >= 0 && id < LABEL_COUNT) ? names[id] : "?";
    }

    inline bool* labelEnabled(int id) {
        switch (id) {
            case LblCheat: return &s.lblCheat;
            case LblFps: return &s.lblFps;
            case LblCps: return &s.lblCps;
            case LblNoclipAcc: return &s.lblNoclipAcc;
            case LblNoclipDeaths: return &s.lblNoclipDeaths;
            case LblAttempts: return &s.lblAttempts;
            case LblJumps: return &s.lblJumps;
            case LblTime: return &s.lblTime;
            case LblClock: return &s.lblClock;
            case LblBestRun: return &s.lblBestRun;
            default: return &s.lblMessage;
        }
    }

    // Keeps the saved order valid: no duplicates, every label exactly once
    inline void fixLabelOrder() {
        std::vector<int> fixed;
        std::vector<bool> seen(LABEL_COUNT, false);
        for (int id : s.labelOrder) {
            if (id >= 0 && id < LABEL_COUNT && !seen[id]) { seen[id] = true; fixed.push_back(id); }
        }
        for (int id = 0; id < LABEL_COUNT; id++) if (!seen[id]) fixed.push_back(id);
        s.labelOrder = fixed;
    }

    // Stats for the current attempt (reset on every new attempt)
    struct Stats {
        int totalFrames = 0;
        int deadFrames = 0;
        int deaths = 0;
        bool diedThisFrame = false;
        bool wasDying = false;
        bool cheated = false;     // a cheat was used at some point in this attempt
        int cps = 0;
        int maxCps = 0;

        void reset() {
            totalFrames = deadFrames = deaths = 0;
            diedThisFrame = wasDying = cheated = false;
            maxCps = 0;
        }
        float noclipAccuracy() const {
            if (totalFrames == 0) return 100.f;
            return 100.f * (1.f - float(deadFrames) / float(totalFrames));
        }
    };

    inline Stats stats;

    // Best run of the current level session (from a start % to a death %)
    struct BestRun {
        float start = 0.f;      // where the current attempt started
        float bestFrom = 0.f;
        float bestTo = -1.f;    // -1: no run yet
        void clear() { start = 0.f; bestFrom = 0.f; bestTo = -1.f; }
        void onRunEnd(float end) {
            if (end - start > bestTo - bestFrom) { bestFrom = start; bestTo = end; }
        }
    };

    inline BestRun bestRun;

    // Set by the bot (Bot.cpp)
    inline bool botPlaying = false;
    inline bool botStepper = false;

    // A cheat is turned on right now
    inline bool cheating() {
        return s.noclip || (s.speedhack && s.speed != 1.0f) || botPlaying || botStepper;
    }

    // Counts a click for the CPS label (Hacks.cpp)
    void onClick();

    // How to open / close the menu on this platform
#ifdef GEODE_IS_MOBILE
    inline constexpr char const* CLOSE_HINT = "Tap the gem button to close";
    inline constexpr char const* OPEN_HINT = "Open / close the menu: tap the gem button (drag it to move it)";
#else
    inline constexpr char const* CLOSE_HINT = "Press the key above Tab to close";
    inline constexpr char const* OPEN_HINT = "Open / close the menu: key above Tab (Escape also closes it)";
#endif

    // The mouse cursor only exists on PC
    inline void showCursor() {
#ifdef GEODE_IS_DESKTOP
        PlatformToolbox::showCursor();
#endif
    }

    // A cheat is on now, or was used earlier in this attempt
    inline bool attemptCheated() {
        return cheating() || stats.cheated;
    }

    inline bool safeActive() {
        return s.safeMode || (s.autoSafe && attemptCheated());
    }

    void load();
    void save();
}
