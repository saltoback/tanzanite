// Tanzanite — touche ², hacks et labels façon Mega Hack
#include "Menu.hpp"
#include "State.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/utils/Keyboard.hpp>

#include <chrono>
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
// Sauvegarde des options
// ---------------------------------------------------------------------------
void tanzanite::load() {
    auto m = Mod::get();
    s.noclip          = m->getSavedValue<bool>("noclip", false);
    s.speedhack       = m->getSavedValue<bool>("speedhack", false);
    s.speed           = m->getSavedValue<float>("speed", 1.0f);
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
    s.message         = m->getSavedValue<std::string>("message", "Tanzanite");
    s.corner          = m->getSavedValue<int>("corner", TopLeft);
    s.labelScale      = m->getSavedValue<float>("label-scale", 1.0f);
    s.labelOpacity    = m->getSavedValue<int>("label-opacity", 180);
}

void tanzanite::save() {
    auto m = Mod::get();
    m->setSavedValue("noclip", s.noclip);
    m->setSavedValue("speedhack", s.speedhack);
    m->setSavedValue("speed", s.speed);
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
    m->setSavedValue("message", s.message);
    m->setSavedValue("corner", s.corner);
    m->setSavedValue("label-scale", s.labelScale);
    m->setSavedValue("label-opacity", s.labelOpacity);
}

// ---------------------------------------------------------------------------
// Touche ² : c'est la touche physique à gauche du 1 (scancode 0x29),
// quelle que soit la disposition du clavier. Sur AZERTY son code est VK_OEM_7.
// ---------------------------------------------------------------------------
$on_mod(Loaded) {
    tanzanite::load();

    KeyboardInputEvent().listen([](KeyboardInputData& data) -> bool {
        if (data.action != KeyboardInputData::Action::Press) return ListenerResult::Propagate;

        bool isSquareKey = data.native.extra == 0x29 || data.native.code == 0xDE;
        if (!isSquareKey) return ListenerResult::Propagate;

        // Ne pas réagir pendant qu'on écrit dans un champ de texte
        if (CCIMEDispatcher::sharedDispatcher()->hasDelegate()) return ListenerResult::Propagate;

        geode::queueInMainThread([] { tanzanite::toggleMenu(); });
        return ListenerResult::Stop;
    }).leak();
}

// ---------------------------------------------------------------------------
// Speedhack : on multiplie le temps écoulé de toute la boucle du jeu
// ---------------------------------------------------------------------------
class $modify(CCScheduler) {
    void update(float dt) {
        if (s.speedhack) dt *= s.speed;
        CCScheduler::update(dt);
    }
};

// ---------------------------------------------------------------------------
// Clics par seconde
// ---------------------------------------------------------------------------
static std::deque<Clock::time_point> s_clicks;

class $modify(GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
        if (down) s_clicks.push_back(Clock::now());
    }
};

// ---------------------------------------------------------------------------
// Labels affichés pendant le niveau
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
        // FPS en temps réel (indépendant du speedhack)
        auto now = Clock::now();
        m_acc += std::chrono::duration<double>(now - m_last).count();
        m_last = now;
        m_frames++;
        if (m_acc >= 0.25) {
            m_fps = static_cast<float>(m_frames / m_acc);
            m_frames = 0;
            m_acc = 0.0;
        }

        // CPS : clics de la dernière seconde
        while (!s_clicks.empty() && now - s_clicks.front() > std::chrono::seconds(1)) {
            s_clicks.pop_front();
        }
        stats.cps = static_cast<int>(s_clicks.size());
        if (stats.cps > stats.maxCps) stats.maxCps = stats.cps;

        // Lignes à afficher
        std::vector<std::pair<std::string, ccColor3B>> lines;
        const ccColor3B white = {255, 255, 255};
        if (s.lblCheat) {
            if (cheating()) {
                lines.push_back({safeActive() ? "Cheats (safe mode)" : "Cheats", {255, 80, 80}});
            } else {
                lines.push_back({"Legit", {120, 255, 150}});
            }
        }
        if (s.lblFps) lines.push_back({fmt::format("FPS : {}", static_cast<int>(m_fps + 0.5f)), white});
        if (s.lblCps) lines.push_back({fmt::format("CPS : {} / {}", stats.cps, stats.maxCps), white});
        if (s.lblNoclipAcc) lines.push_back({fmt::format("Precision : {:.2f}%", stats.noclipAccuracy()), white});
        if (s.lblNoclipDeaths) lines.push_back({fmt::format("Morts : {}", stats.deaths), white});
        if (s.lblAttempts) lines.push_back({fmt::format("Essai {}", m_pl->m_attempts), white});
        if (s.lblJumps) lines.push_back({fmt::format("Sauts : {}", m_pl->m_jumps), white});
        if (s.lblTime) lines.push_back({formatTime(m_pl->m_gameState.m_levelTime), white});
        if (s.lblClock) lines.push_back({clockString(), white});
        if (s.lblMessage && !s.message.empty()) lines.push_back({s.message, PURPLE});

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
// Hooks du niveau : noclip, safe mode, pourcentage précis, stats
// ---------------------------------------------------------------------------
class $modify(TanzanitePlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        stats.reset();
        s_clicks.clear();
        if (auto labels = LabelsNode::create(this)) this->addChild(labels, 1000);
        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        stats.reset();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        if (!m_isPaused && !m_hasCompletedLevel) {
            stats.totalFrames++;
            if (stats.diedThisFrame) {
                stats.deadFrames++;
                if (!stats.wasDying) stats.deaths++;
            }
            stats.wasDying = stats.diedThisFrame;
        }
        stats.diedThisFrame = false;
    }

    // Noclip : on ignore la mort (sauf le pic anti-triche du jeu)
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (s.noclip && object != m_anticheatSpike) {
            stats.diedThisFrame = true;
            return;
        }
        if (safeActive()) m_isTestMode = true;
        PlayLayer::destroyPlayer(player, object);
    }

    // Safe mode : le niveau compte comme un test → rien n'est enregistré
    void levelComplete() {
        if (safeActive()) m_isTestMode = true;
        PlayLayer::levelComplete();
    }

    // Pourcentage avec 2 décimales
    void updateProgressbar() {
        PlayLayer::updateProgressbar();
        if (s.accuratePct && m_percentageLabel) {
            m_percentageLabel->setString(fmt::format("{:.2f}%", this->getCurrentPercent()).c_str());
        }
    }
};
