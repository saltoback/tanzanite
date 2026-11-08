// Tanzanite — macro bot (record / play) and frame stepper
#include "Bot.hpp"
#include "State.hpp"

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/ui/Notification.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <unordered_map>

using namespace geode::prelude;
using namespace tanzanite;
namespace fs = std::filesystem;

namespace {
    constexpr double TPS = 240.0;
    constexpr int FIX_INTERVAL = 60;   // periodic position fix every 0.25 s of level time

    bool s_injecting = false;          // true while the bot itself presses buttons
    bool s_waitForReset = false;       // recording starts at the next attempt
    size_t s_fixIndex = 0;
    bool s_musicPaused = false;

    // Physics tick of the level, derived from the level time (not real time)
    int currentFrame(GJBaseGameLayer* gl) {
        return static_cast<int>(gl->m_gameState.m_levelTime * TPS + 1e-6) + 1;
    }

    bool isPlayLayer(GJBaseGameLayer* gl) {
        auto pl = PlayLayer::get();
        return pl && static_cast<GJBaseGameLayer*>(pl) == gl;
    }

    // --- Position fix -----------------------------------------------------

    bot::PlayerState capture(PlayerObject* p) {
        auto pos = p->getPosition();
        return {pos.x, pos.y, p->getRotation(), p->m_yVelocity, p->m_platformerXVelocity};
    }

    void apply(PlayerObject* p, bot::PlayerState const& st) {
        p->setPosition({st.x, st.y});
        p->setRotation(st.rotation);
        p->m_yVelocity = st.yVel;
        p->m_platformerXVelocity = st.xVel;
    }

    void recordFix(GJBaseGameLayer* gl, int frame) {
        auto& fixes = bot::macro.fixes;
        if (!fixes.empty() && fixes.back().frame >= frame) return;   // one fix per frame
        fixes.push_back({frame, capture(gl->m_player1), capture(gl->m_player2)});
    }

    // --- Full player snapshot for practice checkpoints ---------------------

#define TZ_PLAYER_FIELDS(X) \
    X(position) X(lastGroundedPos) X(yVelocity) X(platformerXVelocity) X(fallSpeed) \
    X(gravity) X(gravityMod) X(isOnGround) X(isOnGround2) X(isOnGround3) X(isOnGround4) \
    X(isUpsideDown) X(isDashing) X(dashX) X(dashY) X(dashAngle) X(dashStartTime) \
    X(vehicleSize) X(playerSpeed) X(isSliding) X(isSlidingRight) X(isRotating) \
    X(rotationSpeed) X(lastJumpTime) X(lastFlipTime) X(lastLandTime) X(isAccelerating) \
    X(accelerationOrSpeed) X(jumpBuffered) X(stateJumpBuffered) X(yVelocityBeforeSlope) \
    X(currentSlopeYVelocity) X(groundYVelocity) X(lastSpiderFlipTime) X(isGoingLeft) \
    X(hasEverJumped)

    struct FullState {
#define TZ_DECLARE(f) decltype(PlayerObject::m_##f) f{};
        TZ_PLAYER_FIELDS(TZ_DECLARE)
#undef TZ_DECLARE
        CCPoint nodePos;
        float rotation = 0.f;
    };

    FullState snapshot(PlayerObject* p) {
        FullState st;
#define TZ_SAVE(f) st.f = p->m_##f;
        TZ_PLAYER_FIELDS(TZ_SAVE)
#undef TZ_SAVE
        st.nodePos = p->getPosition();
        st.rotation = p->getRotation();
        return st;
    }

    void restore(PlayerObject* p, FullState const& st) {
        p->setPosition(st.nodePos);
        p->setRotation(st.rotation);
#define TZ_LOAD(f) p->m_##f = st.f;
        TZ_PLAYER_FIELDS(TZ_LOAD)
#undef TZ_LOAD
    }

    struct CheckpointData {
        int frame;
        FullState p1, p2;
    };
    std::unordered_map<CheckpointObject*, CheckpointData> s_checkpoints;

    // --- Helpers ----------------------------------------------------------

    std::string sanitize(std::string const& name) {
        std::string out;
        for (unsigned char c : name) {
            if (std::isalnum(c) || c == ' ' || c == '-' || c == '_') out += static_cast<char>(c);
            else out += '_';
        }
        if (out.empty()) out = "macro";
        return out;
    }

    // Which buttons the macro is holding right before a frame
    std::map<std::pair<int, bool>, bool> heldBefore(int frame) {
        std::map<std::pair<int, bool>, bool> held;
        for (auto const& in : bot::macro.inputs) {
            if (in.frame >= frame) break;
            held[{in.button, in.player1}] = in.down;
        }
        return held;
    }

    void press(GJBaseGameLayer* gl, bool down, int button, bool player1) {
        s_injecting = true;
        gl->handleButton(down, button, player1);
        s_injecting = false;
    }

    // Play every input (and the position fix) due at this frame
    void playFrame(GJBaseGameLayer* gl, int frame) {
        if (gl->m_player1->m_isDead || gl->m_levelEndAnimationStarted) return;
        auto& m = bot::macro;

        while (s_fixIndex < m.fixes.size() && m.fixes[s_fixIndex].frame <= frame) {
            auto const& fix = m.fixes[s_fixIndex];
            // Only an exact match: a late fix would teleport the player backwards
            if (bot::positionFix && fix.frame == frame) {
                apply(gl->m_player1, fix.p1);
                if (gl->m_gameState.m_isDualMode) apply(gl->m_player2, fix.p2);
            }
            s_fixIndex++;
        }

        while (bot::playIndex < m.inputs.size() && m.inputs[bot::playIndex].frame <= frame) {
            auto const& in = m.inputs[bot::playIndex];
            press(gl, in.down, in.button, in.player1);
            bot::playIndex++;
        }
    }

    void notify(std::string const& text, NotificationIcon icon) {
        Notification::create(text, icon)->show();
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void bot::setMode(Mode m) {
    mode = m;
    playIndex = 0;
    s_fixIndex = 0;
    tanzanite::botPlaying = (m == Mode::Play);
    s_waitForReset = false;
    if (m == Mode::Record) macro.clear();

    // Switched while inside a level
    if (auto pl = PlayLayer::get()) {
        if (m == Mode::Record) {
            macro.levelKey = levelKey(pl->m_level);
            macro.name = std::string(pl->m_level->m_levelName);
            s_waitForReset = true;   // a macro must start at the beginning of an attempt
        } else if (m == Mode::Play) {
            onReset(pl);
        }
    }
}

void bot::setStepper(bool on) {
    stepper = on;
    stepRequests = 0;
    tanzanite::botStepper = on;
    if (!on && s_musicPaused) {
        FMODAudioEngine::sharedEngine()->resumeAllMusic();
        s_musicPaused = false;
    }
}

fs::path bot::macroDir() {
    auto dir = Mod::get()->getSaveDir() / "macros";
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir;
}

std::vector<fs::path> bot::listMacros() {
    std::vector<fs::path> out;
    std::error_code ec;
    for (auto const& entry : fs::directory_iterator(macroDir(), ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".tzm") out.push_back(entry.path());
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::string bot::levelKey(GJGameLevel* level) {
    if (!level) return "unknown";
    int id = level->m_levelID.value();
    if (id > 0) return std::to_string(id);
    return "local-" + sanitize(std::string(level->m_levelName));
}

bool bot::saveMacro(std::string const& name) {
    if (macro.inputs.empty()) return false;
    macro.name = name.empty() ? "macro" : name;
    if (macro.levelKey.empty()) macro.levelKey = "unknown";

    auto path = macroDir() / (sanitize(macro.name) + " [" + macro.levelKey + "].tzm");
    std::ofstream file(path);
    if (!file) return false;

    file << std::setprecision(17);
    file << "TZM 1\n";
    file << "name " << macro.name << "\n";
    file << "level " << macro.levelKey << "\n";
    for (auto const& in : macro.inputs) {
        file << "i " << in.frame << ' ' << in.button << ' ' << in.down << ' ' << in.player1 << "\n";
    }
    for (auto const& fx : macro.fixes) {
        file << "f " << fx.frame
             << ' ' << fx.p1.x << ' ' << fx.p1.y << ' ' << fx.p1.rotation << ' ' << fx.p1.yVel << ' ' << fx.p1.xVel
             << ' ' << fx.p2.x << ' ' << fx.p2.y << ' ' << fx.p2.rotation << ' ' << fx.p2.yVel << ' ' << fx.p2.xVel
             << "\n";
    }
    return static_cast<bool>(file);
}

bool bot::loadMacro(fs::path const& path) {
    std::ifstream file(path);
    if (!file) return false;

    std::string header;
    std::getline(file, header);
    if (header.rfind("TZM", 0) != 0) return false;

    Macro loaded;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string tag;
        ss >> tag;
        if (tag == "name") {
            std::getline(ss >> std::ws, loaded.name);
        } else if (tag == "level") {
            ss >> loaded.levelKey;
        } else if (tag == "i") {
            Input in{};
            int down = 0, p1 = 1;
            ss >> in.frame >> in.button >> down >> p1;
            in.down = down != 0;
            in.player1 = p1 != 0;
            if (ss) loaded.inputs.push_back(in);
        } else if (tag == "f") {
            Fix fx{};
            ss >> fx.frame
               >> fx.p1.x >> fx.p1.y >> fx.p1.rotation >> fx.p1.yVel >> fx.p1.xVel
               >> fx.p2.x >> fx.p2.y >> fx.p2.rotation >> fx.p2.yVel >> fx.p2.xVel;
            if (ss) loaded.fixes.push_back(fx);
        }
    }

    // Stable order by frame, so playback can walk through them
    std::stable_sort(loaded.inputs.begin(), loaded.inputs.end(),
        [](Input const& a, Input const& b) { return a.frame < b.frame; });
    std::stable_sort(loaded.fixes.begin(), loaded.fixes.end(),
        [](Fix const& a, Fix const& b) { return a.frame < b.frame; });

    macro = std::move(loaded);
    playIndex = 0;
    s_fixIndex = 0;
    return true;
}

std::string bot::macroInfo() {
    if (macro.inputs.empty()) return "No macro loaded";
    return fmt::format("{} - {} inputs - {:.1f}s",
        macro.name.empty() ? "Unsaved macro" : macro.name,
        macro.inputs.size(), macro.lastFrame() / TPS);
}

void bot::loadSettings() {
    positionFix = Mod::get()->getSavedValue<bool>("bot-position-fix", true);
}

void bot::saveSettings() {
    Mod::get()->setSavedValue("bot-position-fix", positionFix);
}

// ---------------------------------------------------------------------------
// Level events (called from the PlayLayer hooks in Hacks.cpp)
// ---------------------------------------------------------------------------
void bot::onLevelInit(PlayLayer* pl) {
    s_checkpoints.clear();
    s_musicPaused = false;
    stepRequests = 0;
    auto key = levelKey(pl->m_level);

    if (mode == Mode::Record) {
        macro.clear();
        macro.levelKey = key;
        macro.name = std::string(pl->m_level->m_levelName);
    } else if (mode == Mode::Play && macro.levelKey != key) {
        // Load the macro made for this level automatically
        bool found = false;
        for (auto const& path : listMacros()) {
            if (path.filename().string().find("[" + key + "]") != std::string::npos && loadMacro(path)) {
                found = true;
                break;
            }
        }
        if (found) notify("Macro loaded: " + macro.name, NotificationIcon::Success);
        else notify("No macro saved for this level", NotificationIcon::Warning);
    }
}

void bot::onReset(PlayLayer* pl) {
    if (mode == Mode::Off) return;
    s_waitForReset = false;
    int frame = currentFrame(pl);

    if (mode == Mode::Record) {
        // Respawn: forget everything recorded after this point
        auto& in = macro.inputs;
        in.erase(std::remove_if(in.begin(), in.end(), [&](Input const& x) { return x.frame >= frame; }), in.end());
        auto& fx = macro.fixes;
        fx.erase(std::remove_if(fx.begin(), fx.end(), [&](Fix const& x) { return x.frame >= frame; }), fx.end());

        // Release whatever the macro was holding, in the macro and in the game
        for (auto const& [key, down] : heldBefore(frame)) {
            if (!down) continue;
            macro.inputs.push_back({frame, key.first, false, key.second});
            press(pl, false, key.first, key.second);
        }
        return;
    }

    // Play: continue from the current frame (start of level or checkpoint)
    pl->m_player1->releaseAllButtons();
    pl->m_player2->releaseAllButtons();

    auto const& in = macro.inputs;
    playIndex = std::lower_bound(in.begin(), in.end(), frame,
        [](Input const& x, int f) { return x.frame < f; }) - in.begin();
    auto const& fx = macro.fixes;
    s_fixIndex = std::lower_bound(fx.begin(), fx.end(), frame,
        [](Fix const& x, int f) { return x.frame < f; }) - fx.begin();

    for (auto const& [key, down] : heldBefore(frame)) {
        if (down) press(pl, true, key.first, key.second);
    }
}

void bot::onLevelComplete(PlayLayer* pl) {
    if (mode != Mode::Record || macro.inputs.empty()) return;
    if (saveMacro(macro.name)) notify("Macro saved: " + macro.name, NotificationIcon::Success);
    else notify("Could not save the macro", NotificationIcon::Error);
}

void bot::onQuit() {
    s_checkpoints.clear();
    stepRequests = 0;
    if (s_musicPaused) {
        FMODAudioEngine::sharedEngine()->resumeAllMusic();
        s_musicPaused = false;
    }
}

// ---------------------------------------------------------------------------
// Hooks
// ---------------------------------------------------------------------------
class $modify(BotGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        if (!isPlayLayer(this)) return GJBaseGameLayer::handleButton(down, button, isPlayer1);

        if (down) tanzanite::onClick();

        // While playing, the macro is in control: ignore the player's own inputs
        if (bot::mode == bot::Mode::Play && !s_injecting) return;

        if (bot::mode == bot::Mode::Record && !s_injecting && !s_waitForReset && !m_player1->m_isDead) {
            int frame = currentFrame(this);
            recordFix(this, frame);
            bot::macro.inputs.push_back({frame, button, down, isPlayer1});
        }

        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    // Inputs are processed here, once per physics step: play the macro at the
    // exact same point where the inputs were recorded.
    void processQueuedButtons(float dt, bool clearInputQueue) {
        if (isPlayLayer(this) && bot::mode != bot::Mode::Off) {
            int frame = currentFrame(this);
            if (bot::mode == bot::Mode::Play) {
                playFrame(this, frame);
            } else if (!s_waitForReset && !m_player1->m_isDead) {
                auto const& fixes = bot::macro.fixes;
                if (fixes.empty() || frame - fixes.back().frame >= FIX_INTERVAL) recordFix(this, frame);
            }
        }
        GJBaseGameLayer::processQueuedButtons(dt, clearInputQueue);
    }

    // Safety net: catch any input that was not played during this step
    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        if (isPlayLayer(this) && bot::mode == bot::Mode::Play) {
            playFrame(this, currentFrame(this) - 1);
        }
    }

    // Frame stepper: the game only advances one physics tick per request
    void update(float dt) {
        auto pl = PlayLayer::get();
        if (!bot::stepper || !isPlayLayer(this) || pl->m_isPaused) {
            return GJBaseGameLayer::update(dt);
        }
        if (!s_musicPaused) {
            FMODAudioEngine::sharedEngine()->pauseAllMusic(true);
            s_musicPaused = true;
        }
        if (bot::stepRequests > 0) {
            bot::stepRequests--;
            GJBaseGameLayer::update(static_cast<float>(1.0 / TPS));
        }
    }
};

class $modify(BotPlayLayer, PlayLayer) {
    // Practice checkpoints: keep an exact copy of both players
    void storeCheckpoint(CheckpointObject* cp) {
        PlayLayer::storeCheckpoint(cp);
        if (!cp || bot::mode == bot::Mode::Off) return;
        s_checkpoints[cp] = {currentFrame(this), snapshot(m_player1), snapshot(m_player2)};
    }

    void loadFromCheckpoint(CheckpointObject* cp) {
        PlayLayer::loadFromCheckpoint(cp);
        if (!cp || bot::mode == bot::Mode::Off) return;
        auto it = s_checkpoints.find(cp);
        if (it == s_checkpoints.end()) return;
        restore(m_player1, it->second.p1);
        restore(m_player2, it->second.p2);
    }
};
