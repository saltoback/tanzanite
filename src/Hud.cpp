// Tanzanite — macro mode: the in-level macro interface
#include "Hud.hpp"
#include "Bot.hpp"
#include "State.hpp"
#include "Ui.hpp"

#include <Geode/modify/UILayer.hpp>
#include <Geode/ui/Notification.hpp>

#include <array>
#include <cstring>
#include <functional>

using namespace geode::prelude;
using namespace tanzanite;
using namespace tanzanite::ui;

namespace {
    constexpr float BW = 470.f;    // bar width
    constexpr float BH = 48.f;     // bar height

    constexpr std::array<float, 11> SPEEDS = {0.1f, 0.25f, 0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f, 3.f, 4.f, 8.f};

    void notify(std::string const& text, NotificationIcon icon = NotificationIcon::Info) {
        Notification::create(text, icon)->show();
    }

    // Switch Record / Play from inside a level: practice mode for recording,
    // normal mode for playing, and restart so the macro starts at the beginning.
    void switchMode(bot::Mode m) {
        auto pl = PlayLayer::get();
        if (m == bot::Mode::Play && pl && !bot::macro.levelKey.empty()
            && bot::macro.levelKey != bot::levelKey(pl->m_level)) {
            bot::macro.clear();
        }
        if (m == bot::Mode::Play && pl && bot::macro.inputs.empty()) {
            if (bot::loadForLevel(pl->m_level)) notify("Macro loaded: " + bot::macro.name, NotificationIcon::Success);
        }
        if (m == bot::Mode::Play && bot::macro.inputs.empty()) {
            notify("No macro for this level yet: record one first", NotificationIcon::Warning);
            return;
        }

        bot::setMode(m);
        if (!pl) return;
        if (m == bot::Mode::Record && !pl->m_isPracticeMode) pl->togglePracticeMode(true);
        if (m == bot::Mode::Play && pl->m_isPracticeMode) pl->togglePracticeMode(false);
        pl->resetLevelFromStart();
    }

    void stepSpeed(int dir) {
        // Find the closest preset, then move one step
        size_t idx = 0;
        float best = 1e9f;
        for (size_t i = 0; i < SPEEDS.size(); i++) {
            float d = std::abs(SPEEDS[i] - s.speed);
            if (d < best) { best = d; idx = i; }
        }
        int next = std::clamp(int(idx) + dir, 0, int(SPEEDS.size()) - 1);
        s.speed = SPEEDS[next];
        s.speedhack = s.speed != 1.f;
        tanzanite::save();
    }
}

class MacroHud : public CCNode {
    CCMenu* m_menu = nullptr;
    CCLabelBMFont* m_info = nullptr;
    PillNode* m_record = nullptr;
    PillNode* m_play = nullptr;
    PillNode* m_pause = nullptr;
    PillNode* m_speed = nullptr;
    PillNode* m_hitboxes = nullptr;
    PillNode* m_trajectory = nullptr;
    float m_cursorTimer = 0.f;
    std::string m_lastSpeed;

    PillNode* addPill(char const* text, float x, float y, float w, std::function<void()> cb) {
        auto pill = PillNode::create(text, w, 16.f, 0.3f);
        auto item = CCMenuItemExt::createSpriteExtra(pill, [cb](auto) {
            cb();
            PlatformToolbox::showCursor();
        });
        item->setPosition({x + w / 2.f, y});
        m_menu->addChild(item);
        return pill;
    }

public:
    static MacroHud* create() {
        auto ret = new MacroHud();
        if (ret->init()) { ret->autorelease(); return ret; }
        delete ret;
        return nullptr;
    }

    bool init() override {
        if (!CCNode::init()) return false;
        this->setID("macro-hud"_spr);
        this->setContentSize({BW, BH});

        auto win = CCDirector::get()->getWinSize();
        this->setPosition({(win.width - BW) / 2.f, 4.f});

        auto bg = makeDraw();
        drawRoundedRect(bg, {0.f, 0.f, BW, BH}, 9.f, PANEL4, 1.4f, PURPLE4);
        bg->drawSegment({BW * 0.f + 8.f, BH / 2.f}, {BW - 8.f, BH / 2.f}, 0.4f, rgba(0.59f, 0.43f, 1.f, 0.35f));
        this->addChild(bg);

        m_menu = CCMenu::create();
        m_menu->ignoreAnchorPointForPosition(false);
        m_menu->setAnchorPoint({0.f, 0.f});
        m_menu->setContentSize({BW, BH});
        m_menu->setPosition({0.f, 0.f});
        this->addChild(m_menu);

        m_info = makeLabel("", "bigFont.fnt", 0.3f, {230, 220, 255});
        m_info->setAnchorPoint({0.f, 0.f});
        m_info->setPosition({4.f, BH + 4.f});
        this->addChild(m_info);

        // Row 1: mode, pause / step, restart / save / exit
        float y = BH - 12.f;
        float x = 8.f;
        m_record = addPill("Record", x, y, 54.f, [] { switchMode(bot::Mode::Record); });  x += 58.f;
        m_play   = addPill("Play", x, y, 46.f, [] { switchMode(bot::Mode::Play); });      x += 62.f;
        m_pause  = addPill("Pause", x, y, 56.f, [] { bot::setStepper(!bot::stepper); });  x += 60.f;
        addPill("+1 Frame", x, y, 64.f, [] {
            if (!bot::stepper) bot::setStepper(true);
            bot::stepRequests++;
        });
        x += 80.f;
        addPill("Restart", x, y, 56.f, [] {
            if (auto pl = PlayLayer::get()) pl->resetLevelFromStart();
        });
        x += 60.f;
        addPill("Save", x, y, 44.f, [] {
            if (bot::macro.inputs.empty()) notify("Nothing to save yet", NotificationIcon::Warning);
            else if (bot::saveMacro(bot::macro.name)) notify("Macro saved: " + bot::macro.name, NotificationIcon::Success);
            else notify("Could not save the macro", NotificationIcon::Error);
        });
        x += 48.f;
        addPill("Exit", x, y, 44.f, [] { hud::exitMacroMode(); });

        // Row 2: speed, hitboxes, trajectory, checkpoints
        y = 12.f;
        x = 8.f;
        auto speedLbl = makeLabel("Speed", "bigFont.fnt", 0.28f, {200, 190, 235});
        speedLbl->setAnchorPoint({0.f, 0.5f});
        speedLbl->setPosition({x, y});
        this->addChild(speedLbl);
        x += 40.f;
        addPill("-", x, y, 20.f, [] { stepSpeed(-1); });                  x += 22.f;
        m_speed = addPill("1.00x", x, y, 52.f, [] {
            s.speedhack = !s.speedhack;
            tanzanite::save();
        });
        x += 54.f;
        addPill("+", x, y, 20.f, [] { stepSpeed(1); });                   x += 34.f;
        m_hitboxes = addPill("Hitboxes", x, y, 66.f, [] {
            s.showHitboxes = !s.showHitboxes;
            tanzanite::save();
        });
        x += 70.f;
        m_trajectory = addPill("Trajectory", x, y, 74.f, [] {
            s.showTrajectory = !s.showTrajectory;
            tanzanite::save();
        });
        x += 88.f;
        addPill("+ CP", x, y, 44.f, [] {
            auto pl = PlayLayer::get();
            if (!pl) return;
            if (!pl->m_isPracticeMode) notify("Checkpoints need practice mode (use Record)", NotificationIcon::Warning);
            else pl->markCheckpoint();
        });
        x += 48.f;
        addPill("- CP", x, y, 44.f, [] {
            auto pl = PlayLayer::get();
            if (pl && pl->m_isPracticeMode) pl->removeCheckpoint(false);
        });

        this->scheduleUpdate();
        this->update(0.f);
        PlatformToolbox::showCursor();
        return true;
    }

    void update(float dt) override {
        m_record->setSelected(bot::mode == bot::Mode::Record);
        m_play->setSelected(bot::mode == bot::Mode::Play);
        m_pause->setSelected(bot::stepper);
        m_hitboxes->setSelected(s.showHitboxes);
        m_trajectory->setSelected(s.showTrajectory);
        m_speed->setSelected(s.speedhack);

        auto speedText = s.speedhack ? fmt::format("{:.2f}x", s.speed) : std::string("1.00x");
        if (speedText != m_lastSpeed) {
            m_speed->setText(speedText.c_str());
            m_lastSpeed = speedText;
        }
        m_pause->setText(bot::stepper ? "Resume" : "Pause");

        // Status line above the bar
        std::string info;
        auto pl = PlayLayer::get();
        int frame = pl ? static_cast<int>(pl->m_gameState.m_levelTime * 240.0 + 1e-6) + 1 : 0;
        switch (bot::mode) {
            case bot::Mode::Record:
                info = fmt::format("RECORDING  -  {} inputs  -  frame {}", bot::macro.inputs.size(), frame);
                break;
            case bot::Mode::Play:
                info = fmt::format("PLAYING  -  {} / {} inputs  -  frame {}", bot::playIndex, bot::macro.inputs.size(), frame);
                break;
            default:
                info = "MACRO MODE  -  choose Record or Play";
                break;
        }
        if (bot::stepper) info += "  -  PAUSED (F or +1 Frame to step)";
        if (std::strcmp(m_info->getString(), info.c_str()) != 0) m_info->setString(info.c_str());

        // Keep the mouse cursor visible while macro mode is on
        m_cursorTimer += dt;
        if (m_cursorTimer > 0.5f) {
            m_cursorTimer = 0.f;
            if (pl && !pl->m_isPaused) PlatformToolbox::showCursor();
        }
    }
};

namespace {
    MacroHud* s_hud = nullptr;

    void removeHud() {
        if (!s_hud) return;
        s_hud->removeFromParent();
        s_hud->release();
        s_hud = nullptr;
    }

    void addHud(PlayLayer* pl) {
        removeHud();
        if (!pl->m_uiLayer) return;
        s_hud = MacroHud::create();
        if (!s_hud) return;
        s_hud->retain();
        pl->m_uiLayer->addChild(s_hud, 1000);
    }
}

void hud::enterMacroMode() {
    bot::macroMode = true;
    auto pl = PlayLayer::get();
    if (!pl) {
        if (bot::mode == bot::Mode::Off) bot::setMode(bot::Mode::Record);
        notify("Macro mode on: open a level to start", NotificationIcon::Success);
        return;
    }
    addHud(pl);
    switchMode(bot::mode == bot::Mode::Play ? bot::Mode::Play : bot::Mode::Record);
}

void hud::exitMacroMode() {
    bot::macroMode = false;
    bot::setMode(bot::Mode::Off);
    bot::setStepper(false);
    removeHud();
}

void hud::onLevelInit(PlayLayer* pl) {
    removeHud();
    if (!bot::macroMode) return;
    addHud(pl);
    if (bot::mode == bot::Mode::Record && !pl->m_isPracticeMode) {
        // After the level finished loading
        geode::queueInMainThread([] {
            auto pl = PlayLayer::get();
            if (pl && bot::macroMode && bot::mode == bot::Mode::Record && !pl->m_isPracticeMode) {
                pl->togglePracticeMode(true);
            }
        });
    }
}

void hud::onQuit() {
    removeHud();
}

bool hud::blocksPoint(CCPoint world) {
    if (!s_hud || !s_hud->getParent()) return false;
    auto origin = s_hud->convertToWorldSpace({0.f, 0.f});
    auto top = s_hud->convertToWorldSpace({BW, BH + 14.f});
    CCRect rect(origin.x - 4.f, origin.y - 4.f, top.x - origin.x + 8.f, top.y - origin.y + 8.f);
    return rect.containsPoint(world);
}

// Clicks on the interface must press its buttons, not make the player jump
class $modify(HudUILayer, UILayer) {
    bool ccTouchBegan(CCTouch* touch, CCEvent* event) {
        if (hud::blocksPoint(touch->getLocation())) return false;
        return UILayer::ccTouchBegan(touch, event);
    }
};
