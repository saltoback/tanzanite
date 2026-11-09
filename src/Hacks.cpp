// Tanzanite — menu key, hacks and Mega Hack style labels
#include "Bot.hpp"
#include "Hud.hpp"
#include "Menu.hpp"
#include "State.hpp"
#include "Trajectory.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/utils/Keyboard.hpp>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <string>
#include <utility>
#include <vector>

using namespace geode::prelude;
using namespace tanzanite;
using Clock = std::chrono::steady_clock;

// ---------------------------------------------------------------------------
// Saving options
// ---------------------------------------------------------------------------
void tanzanite::load() {
    auto m = Mod::get();
    s.noclip          = m->getSavedValue<bool>("noclip", false);
    s.speedhack       = m->getSavedValue<bool>("speedhack", false);
    s.speed           = m->getSavedValue<float>("speed", 1.0f);
    s.showHitboxes    = m->getSavedValue<bool>("show-hitboxes", false);
    s.showTrajectory  = m->getSavedValue<bool>("show-trajectory", false);
    s.safeMode        = m->getSavedValue<bool>("safe-mode", false);
    s.autoSafe        = m->getSavedValue<bool>("auto-safe", true);
    s.accuratePct     = m->getSavedValue<bool>("accurate-pct", false);
    s.lblCheat        = m->getSavedValue<bool>("lbl-cheat", true);
    s.lblFps          = m->getSavedValue<bool>("lbl-fps", false);
    s.lblCps          = m->getSavedValue<bool>("lbl-cps", false);
    s.lblNoclipAcc    = m->getSavedValue<bool>("lbl-noclip-acc", false);
    s.lblNoclipDeaths = m->getSavedValue<bool>("lbl-noclip-deaths", false);
    s.lblAttempts     = m->getSavedValue<bool>("lbl-attempts", false);
    s.lblJumps        = m->getSavedValue<bool>("lbl-jumps", false);
    s.lblTime         = m->getSavedValue<bool>("lbl-time", false);
    s.lblClock        = m->getSavedValue<bool>("lbl-clock", false);
    s.lblMessage      = m->getSavedValue<bool>("lbl-message", false);
    s.lblBestRun      = m->getSavedValue<bool>("lbl-best-run", false);
    s.bestRunAccurate = m->getSavedValue<bool>("best-run-accurate", true);
    s.speedhackMusic  = m->getSavedValue<bool>("speedhack-music", true);
    s.noParticles     = m->getSavedValue<bool>("no-particles", false);
    s.noIconEffects   = m->getSavedValue<bool>("no-icon-effects", false);
    s.labelOrder.clear();
    {
        // Saved as "0,1,2,..."
        auto text = m->getSavedValue<std::string>("label-order", "");
        size_t start = 0;
        while (start < text.size()) {
            size_t end = text.find(',', start);
            if (end == std::string::npos) end = text.size();
            auto part = text.substr(start, end - start);
            if (!part.empty()) s.labelOrder.push_back(std::atoi(part.c_str()));
            start = end + 1;
        }
    }
    fixLabelOrder();
    s.message         = m->getSavedValue<std::string>("message", "Tanzanite");
    s.corner          = m->getSavedValue<int>("corner", TopLeft);
    s.labelScale      = m->getSavedValue<float>("label-scale", 1.0f);
    s.labelOpacity    = m->getSavedValue<int>("label-opacity", 180);
    bot::loadSettings();
}

void tanzanite::save() {
    auto m = Mod::get();
    m->setSavedValue("noclip", s.noclip);
    m->setSavedValue("speedhack", s.speedhack);
    m->setSavedValue("speed", s.speed);
    m->setSavedValue("show-hitboxes", s.showHitboxes);
    m->setSavedValue("show-trajectory", s.showTrajectory);
    m->setSavedValue("safe-mode", s.safeMode);
    m->setSavedValue("auto-safe", s.autoSafe);
    m->setSavedValue("accurate-pct", s.accuratePct);
    m->setSavedValue("lbl-cheat", s.lblCheat);
    m->setSavedValue("lbl-fps", s.lblFps);
    m->setSavedValue("lbl-cps", s.lblCps);
    m->setSavedValue("lbl-noclip-acc", s.lblNoclipAcc);
    m->setSavedValue("lbl-noclip-deaths", s.lblNoclipDeaths);
    m->setSavedValue("lbl-attempts", s.lblAttempts);
    m->setSavedValue("lbl-jumps", s.lblJumps);
    m->setSavedValue("lbl-time", s.lblTime);
    m->setSavedValue("lbl-clock", s.lblClock);
    m->setSavedValue("lbl-message", s.lblMessage);
    m->setSavedValue("lbl-best-run", s.lblBestRun);
    m->setSavedValue("best-run-accurate", s.bestRunAccurate);
    m->setSavedValue("speedhack-music", s.speedhackMusic);
    m->setSavedValue("no-particles", s.noParticles);
    m->setSavedValue("no-icon-effects", s.noIconEffects);
    {
        std::string order;
        for (int id : s.labelOrder) {
            if (!order.empty()) order += ",";
            order += std::to_string(id);
        }
        m->setSavedValue("label-order", order);
    }
    m->setSavedValue("message", s.message);
    m->setSavedValue("corner", s.corner);
    m->setSavedValue("label-scale", s.labelScale);
    m->setSavedValue("label-opacity", s.labelOpacity);
    bot::saveSettings();
}

// ---------------------------------------------------------------------------
// Menu key: the physical key left of "1" (scancode 0x29), whatever the
// keyboard layout. It is "²" on AZERTY (VK_OEM_7) and "`" on QWERTY.
// ---------------------------------------------------------------------------
$on_mod(Loaded) {
    tanzanite::load();

    KeyboardInputEvent().listen([](KeyboardInputData& data) -> bool {
        using Action = KeyboardInputData::Action;
        if (data.action == Action::Release) return ListenerResult::Propagate;

        // Don't react while typing in a text field
        if (CCIMEDispatcher::sharedDispatcher()->hasDelegate()) return ListenerResult::Propagate;

        // Frame stepper: F advances one physics tick (hold it to keep stepping)
        if (bot::stepper && data.key == KEY_F && PlayLayer::get()) {
            bot::stepRequests++;
            return ListenerResult::Stop;
        }

        if (data.action != Action::Press) return ListenerResult::Propagate;

        bool isMenuKey = data.native.extra == 0x29 || data.native.code == 0xDE;
        if (!isMenuKey) return ListenerResult::Propagate;

        geode::queueInMainThread([] { tanzanite::toggleMenu(); });
        return ListenerResult::Stop;
    }).leak();
}

// ---------------------------------------------------------------------------
// Speedhack: scale the delta time of the whole game loop
// ---------------------------------------------------------------------------
class $modify(CCScheduler) {
    void update(float dt) {
        if (s.speedhack) dt *= s.speed;

        // Music follows the game speed (option "Speed up music")
        static float s_pitch = 1.f;
        float pitch = (s.speedhack && s.speedhackMusic) ? s.speed : 1.f;
        if (pitch != s_pitch) {
            auto engine = FMODAudioEngine::sharedEngine();
            FMOD::ChannelGroup* master = nullptr;
            if (engine && engine->m_system && engine->m_system->getMasterChannelGroup(&master) == FMOD_OK && master) {
                master->setPitch(pitch);
                s_pitch = pitch;
            }
        }

        CCScheduler::update(dt);
    }
};

// ---------------------------------------------------------------------------
// Clicks per second
// ---------------------------------------------------------------------------
// (the handleButton hook lives in Bot.cpp and calls this)
static std::deque<Clock::time_point> s_clicks;

void tanzanite::onClick() {
    s_clicks.push_back(Clock::now());
}

// ---------------------------------------------------------------------------
// Labels shown during a level
// ---------------------------------------------------------------------------
class LabelsNode : public CCNode {
    PlayLayer* m_pl = nullptr;
    std::vector<CCLabelBMFont*> m_labels;
    Clock::time_point m_last;
    int m_frames = 0;
    double m_acc = 0.0;
    float m_fps = 0.f;

    static std::string formatTime(double t) {
        int total = static_cast<int>(t);
        int ms = static_cast<int>((t - total) * 1000.0);
        return fmt::format("{}:{:02}.{:03}", total / 60, total % 60, ms);
    }

    static std::string clockString() {
        std::time_t now = std::time(nullptr);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &now);
#else
        localtime_r(&now, &tm);
#endif
        return fmt::format("{:02}:{:02}:{:02}", tm.tm_hour, tm.tm_min, tm.tm_sec);
    }

public:
    static LabelsNode* create(PlayLayer* pl) {
        auto ret = new LabelsNode();
        if (ret->init(pl)) { ret->autorelease(); return ret; }
        delete ret;
        return nullptr;
    }

    bool init(PlayLayer* pl) {
        if (!CCNode::init()) return false;
        m_pl = pl;
        m_last = Clock::now();
        this->setID("labels"_spr);
        this->scheduleUpdate();
        return true;
    }

    void update(float) override {
        // Real-time FPS (not affected by the speedhack)
        auto now = Clock::now();
        m_acc += std::chrono::duration<double>(now - m_last).count();
        m_last = now;
        m_frames++;
        if (m_acc >= 0.25) {
            m_fps = static_cast<float>(m_frames / m_acc);
            m_frames = 0;
            m_acc = 0.0;
        }

        // CPS: clicks during the last second
        while (!s_clicks.empty() && now - s_clicks.front() > std::chrono::seconds(1)) {
            s_clicks.pop_front();
        }
        stats.cps = static_cast<int>(s_clicks.size());
        if (stats.cps > stats.maxCps) stats.maxCps = stats.cps;

        // Lines to show
        std::vector<std::pair<std::string, ccColor3B>> lines;
        const ccColor3B white = {255, 255, 255};
        // In the order chosen in the menu
        for (int id : s.labelOrder) {
            if (!*labelEnabled(id)) continue;
            switch (id) {
                case LblCheat:
                    // Stays on "Cheats" for the whole attempt, even if the cheat is turned off
                    if (attemptCheated()) {
                        lines.push_back({safeActive() ? "Cheats (Safe Mode)" : "Cheats", {255, 80, 80}});
                    } else {
                        lines.push_back({"Legit", {120, 255, 150}});
                    }
                    break;
                case LblFps: lines.push_back({fmt::format("FPS: {}", static_cast<int>(m_fps + 0.5f)), white}); break;
                case LblCps: lines.push_back({fmt::format("CPS: {} / {}", stats.cps, stats.maxCps), white}); break;
                case LblNoclipAcc: lines.push_back({fmt::format("Accuracy: {:.2f}%", stats.noclipAccuracy()), white}); break;
                case LblNoclipDeaths: lines.push_back({fmt::format("Deaths: {}", stats.deaths), white}); break;
                case LblAttempts: lines.push_back({fmt::format("Attempt {}", m_pl->m_attempts), white}); break;
                case LblJumps: lines.push_back({fmt::format("Jumps: {}", m_pl->m_jumps), white}); break;
                case LblTime: lines.push_back({formatTime(m_pl->m_gameState.m_levelTime), white}); break;
                case LblClock: lines.push_back({clockString(), white}); break;
                case LblBestRun: {
                    std::string text = "Best Run: none";
                    if (bestRun.bestTo >= 0.f) {
                        auto pct = [](float v) {
                            return s.bestRunAccurate ? fmt::format("{:.2f}", v) : fmt::format("{}", static_cast<int>(v));
                        };
                        text = bestRun.bestFrom > 0.f
                            ? fmt::format("Best Run: {}-{}%", pct(bestRun.bestFrom), pct(bestRun.bestTo))
                            : fmt::format("Best Run: {}%", pct(bestRun.bestTo));
                    }
                    lines.push_back({text, white});
                    break;
                }
                case LblMessage:
                    if (!s.message.empty()) lines.push_back({s.message, PURPLE});
                    break;
                default: break;
            }
        }

        // Bot status: always shown while the bot or the frame stepper is on
        if (bot::macroMode) {
            // the macro interface already shows the bot status
        } else if (bot::mode == bot::Mode::Record) {
            lines.push_back({fmt::format("Recording - {} inputs", bot::macro.inputs.size()), {255, 110, 110}});
        } else if (bot::mode == bot::Mode::Play) {
            lines.push_back({fmt::format("Playing - {} / {}", bot::playIndex, bot::macro.inputs.size()), PURPLE});
        }
        if (bot::stepper && !bot::macroMode) lines.push_back({"Paused - F: next frame", {255, 220, 120}});

        while (m_labels.size() < lines.size()) {
            auto lbl = CCLabelBMFont::create("", "bigFont.fnt");
            this->addChild(lbl);
            m_labels.push_back(lbl);
        }

        auto win = CCDirector::get()->getWinSize();
        bool top = s.corner == TopLeft || s.corner == TopRight;
        bool left = s.corner == TopLeft || s.corner == BottomLeft;
        float offset = 0.f;

        for (size_t i = 0; i < m_labels.size(); i++) {
            auto lbl = m_labels[i];
            if (i >= lines.size()) {
                lbl->setVisible(false);
                continue;
            }
            auto const& [text, color] = lines[i];
            if (std::strcmp(lbl->getString(), text.c_str()) != 0) lbl->setString(text.c_str());
            lbl->setVisible(true);
            lbl->setColor(color);
            lbl->setOpacity(static_cast<GLubyte>(s.labelOpacity));
            lbl->setScale(0.35f * s.labelScale);
            lbl->setAnchorPoint({left ? 0.f : 1.f, top ? 1.f : 0.f});

            float x = left ? 6.f : win.width - 6.f;
            float y = top ? win.height - 6.f - offset : 6.f + offset;
            lbl->setPosition({x, y});
            offset += lbl->getScaledContentSize().height + 1.f;
        }
    }
};

// ---------------------------------------------------------------------------
// Level hooks: noclip, safe mode, accurate percentage, stats
// ---------------------------------------------------------------------------
class $modify(TanzanitePlayLayer, PlayLayer) {
    struct Fields {
        bool hitboxesOn = false;
        bool particlesHidden = false;
        bool effectsHidden = false;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        stats.reset();
        bestRun.clear();
        s_clicks.clear();
        bot::onLevelInit(this);
        traj::onLevelInit(this);
        hud::onLevelInit(this);
        if (auto labels = LabelsNode::create(this)) this->addChild(labels, 1000);
        return true;
    }

    // New attempt: forget the cheats used in the previous one
    void resetLevel() {
        PlayLayer::resetLevel();
        stats.reset();
        bestRun.start = this->getCurrentPercent();   // start pos / checkpoint
        bot::onReset(this);
    }

    void onQuit() {
        stats.reset();
        bot::onQuit();
        traj::onQuit();
        hud::onQuit();
        PlayLayer::onQuit();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        if (!m_isPaused && !m_hasCompletedLevel) {
            if (cheating()) stats.cheated = true;
            stats.totalFrames++;
            if (stats.diedThisFrame) {
                stats.deadFrames++;
                if (!stats.wasDying) stats.deaths++;
            }
            stats.wasDying = stats.diedThisFrame;
        }
        stats.diedThisFrame = false;

        // Hitboxes (the game's own debug draw)
        if (s.showHitboxes) {
            m_isDebugDrawEnabled = true;
            if (m_debugDrawNode) m_debugDrawNode->setVisible(true);
            this->updateDebugDraw();
            m_fields->hitboxesOn = true;
        } else if (m_fields->hitboxesOn) {
            m_isDebugDrawEnabled = false;
            if (m_debugDrawNode) m_debugDrawNode->clear();
            m_fields->hitboxesOn = false;
        }

        // No Particles / No Icon Effects (made visible again once when turned off)
        bool doParticles = s.noParticles || m_fields->particlesHidden;
        bool doEffects = s.noIconEffects || m_fields->effectsHidden;
        applyPlayerEffects(m_player1, doParticles, doEffects);
        applyPlayerEffects(m_player2, doParticles, doEffects);
        m_fields->particlesHidden = s.noParticles;
        m_fields->effectsHidden = s.noIconEffects;

        traj::update(this);
    }

    // Particles and icon effects of one player, for every game mode
    static void applyPlayerEffects(PlayerObject* p, bool doParticles, bool doEffects) {
        if (!p) return;
        if (doParticles) {
            CCNode* particles[] = {
                p->m_playerGroundParticles, p->m_trailingParticles, p->m_shipClickParticles,
                p->m_vehicleGroundParticles, p->m_ufoClickParticles, p->m_robotBurstParticles,
                p->m_dashParticles, p->m_swingBurstParticles1, p->m_swingBurstParticles2,
                p->m_landParticles0, p->m_landParticles1,
            };
            for (auto n : particles) if (n) n->setVisible(!s.noParticles);
        }
        if (doEffects) {
            CCNode* effects[] = {p->m_regularTrail, p->m_shipStreak, p->m_waveTrail, p->m_ghostTrail};
            for (auto n : effects) if (n) n->setVisible(!s.noIconEffects);
            if (s.noIconEffects && p->m_dashFireSprite) p->m_dashFireSprite->setVisible(false);
        }
    }

    // Noclip: ignore deaths (except the game's own anticheat spike)
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        // The trajectory's invisible player hit something: just stop its path
        if (traj::simulating()) {
            traj::onSimDeath();
            return;
        }
        if (s.noclip && object != m_anticheatSpike) {
            stats.diedThisFrame = true;
            stats.cheated = true;
            return;
        }
        if (safeActive()) m_isTestMode = true;
        if (!m_hasCompletedLevel) bestRun.onRunEnd(this->getCurrentPercent());
        PlayLayer::destroyPlayer(player, object);
    }

    // Safe mode: the level counts as a test run, so nothing is saved.
    // Uses the whole attempt: turning a cheat off before the end doesn't help.
    void levelComplete() {
        if (safeActive()) m_isTestMode = true;
        bot::onLevelComplete(this);
        bestRun.onRunEnd(100.f);
        PlayLayer::levelComplete();
    }

    // Percentage with 2 decimals
    void updateProgressbar() {
        PlayLayer::updateProgressbar();
        if (s.accuratePct && m_percentageLabel) {
            m_percentageLabel->setString(fmt::format("{:.2f}%", this->getCurrentPercent()).c_str());
        }
    }
};
