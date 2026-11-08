// Tanzanite — état partagé (options sauvegardées + statistiques de partie)
#pragma once

#include <Geode/Geode.hpp>
#include <string>

namespace tanzanite {
    using namespace geode::prelude;

    // Couleurs du thème
    inline const ccColor3B PURPLE = {150, 110, 255};
    inline const ccColor4F PURPLE4 = {0.59f, 0.43f, 1.f, 1.f};
    inline const ccColor4F PANEL4 = {0.04f, 0.02f, 0.09f, 0.78f};
    inline const ccColor4F SIDEBAR4 = {0.10f, 0.06f, 0.22f, 0.70f};

    // Coins de l'écran pour les labels
    enum Corner { TopLeft = 0, TopRight, BottomLeft, BottomRight };

    struct State {
        // Joueur
        bool noclip = false;
        bool speedhack = false;
        float speed = 1.0f;
        // Niveau
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
        std::string message = "Tanzanite";
        int corner = TopLeft;
        float labelScale = 1.0f;
        int labelOpacity = 180;
    };

    inline State s;

    // Statistiques de la partie en cours (remises à zéro à chaque essai)
    struct Stats {
        int totalFrames = 0;
        int deadFrames = 0;
        int deaths = 0;
        bool diedThisFrame = false;
        bool wasDying = false;
        int cps = 0;
        int maxCps = 0;

        void reset() {
            totalFrames = deadFrames = deaths = 0;
            diedThisFrame = wasDying = false;
            maxCps = 0;
        }
        float noclipAccuracy() const {
            if (totalFrames == 0) return 100.f;
            return 100.f * (1.f - float(deadFrames) / float(totalFrames));
        }
    };

    inline Stats stats;

    inline bool cheating() {
        return s.noclip || (s.speedhack && s.speed != 1.0f);
    }

    inline bool safeActive() {
        return s.safeMode || (s.autoSafe && cheating());
    }

    void load();
    void save();
}
