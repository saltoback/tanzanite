// Tanzanite — StartPos Switcher
// Collects the level's start positions, sorted from left to right. Switching
// sets the game's own start position object and restarts the attempt there.
#include "StartPos.hpp"
#include "Bot.hpp"
#include "State.hpp"
#include "Ui.hpp"

#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>
#include <cstring>
#include <vector>

using namespace geode::prelude;
using namespace tanzanite;
using namespace tanzanite::ui;

namespace {
    std::vector<StartPosObject*> s_list;   // owned by the level, sorted by x
    int s_index = 0;                       // 0 = level start, i = s_list[i - 1]
    CCNode* s_ui = nullptr;

    constexpr float UW = 150.f;   // switcher width
    constexpr float UH = 22.f;
}

class StartPosUI : public CCNode {
    CCLabelBMFont* m_label = nullptr;

public:
    static StartPosUI* create() {
        auto ret = new StartPosUI();
        if (ret->init()) { ret->autorelease(); return ret; }
        delete ret;
        return nullptr;
    }

    bool init() override {
        if (!CCNode::init()) return false;
        this->setID("startpos-switcher"_spr);
        this->setContentSize({UW, UH});
        auto win = CCDirector::get()->getWinSize();
        this->setPosition({(win.width - UW) / 2.f, 6.f});

        auto bg = makeDraw();
        drawRoundedRect(bg, {0.f, 0.f, UW, UH}, 7.f, PANEL4, 1.2f, PURPLE4);
        this->addChild(bg);

        m_label = makeLabel("", "bigFont.fnt", 0.32f);
        m_label->setPosition({UW / 2.f, UH / 2.f + 1.f});
        this->addChild(m_label);

        auto menu = CCMenu::create();
        menu->setContentSize({UW, UH});
        menu->ignoreAnchorPointForPosition(false);
        menu->setAnchorPoint({0.f, 0.f});
        menu->setPosition({0.f, 0.f});
        this->addChild(menu);
        for (int dir : {-1, 1}) {
            auto spr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
            spr->setScale(0.3f);
            spr->setFlipX(dir < 0);
            auto item = CCMenuItemExt::createSpriteExtra(spr, [dir](auto) {
                geode::queueInMainThread([dir] { startpos::switchBy(dir); });
            });
            item->setPosition({dir < 0 ? 13.f : UW - 13.f, UH / 2.f});
            menu->addChild(item);
        }

        this->scheduleUpdate();
        this->update(0.f);
        return true;
    }

    void update(float) override {
        this->setVisible(s.startposSwitcher && !s_list.empty());
        this->setPositionY(bot::macroMode ? 74.f : 6.f);
        if (!this->isVisible()) return;
        auto text = s_index == 0
            ? fmt::format("Start  (0/{})", s_list.size())
            : fmt::format("StartPos {}/{}", s_index, s_list.size());
        if (std::strcmp(m_label->getString(), text.c_str()) != 0) m_label->setString(text.c_str());
    }
};

void startpos::switchBy(int dir) {
    auto pl = PlayLayer::get();
    if (!pl || s_list.empty() || pl->m_hasCompletedLevel) return;
    int count = int(s_list.size()) + 1;
    s_index = ((s_index + dir) % count + count) % count;
    StartPosObject* sp = s_index == 0 ? nullptr : s_list[s_index - 1];

    pl->m_currentCheckpoint = nullptr;
    pl->setStartPosObject(sp);
    // A start position run never saves progress (same as the game)
    pl->m_isTestMode = sp != nullptr;
    if (pl->m_isPracticeMode) pl->resetLevelFromStart();
    else pl->resetLevel();
    pl->startMusic();
}

bool startpos::blocksPoint(CCPoint world) {
    if (!s_ui || !s_ui->getParent() || !s_ui->isVisible()) return false;
    auto p = s_ui->convertToNodeSpace(world);
    return p.x >= -4.f && p.y >= -4.f && p.x <= UW + 4.f && p.y <= UH + 4.f;
}

class $modify(StartPosPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        s_list.clear();
        s_index = 0;
        s_ui = nullptr;
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }

    void addObject(GameObject* object) {
        PlayLayer::addObject(object);
        if (object && object->m_objectID == 31) s_list.push_back(static_cast<StartPosObject*>(object));
    }

    // All objects exist: sort them and find the one the level started from
    void createObjectsFromSetupFinished() {
        PlayLayer::createObjectsFromSetupFinished();
        std::sort(s_list.begin(), s_list.end(), [](StartPosObject* a, StartPosObject* b) {
            return a->getPositionX() < b->getPositionX();
        });
        s_index = 0;
        for (int i = 0; i < int(s_list.size()); i++) {
            if (s_list[i] == m_startPosObject) s_index = i + 1;
        }
        if (!s_ui && m_uiLayer) {
            if (auto ui = StartPosUI::create()) {
                m_uiLayer->addChild(ui, 999);
                s_ui = ui;
            }
        }
    }

    void onQuit() {
        s_list.clear();
        s_index = 0;
        s_ui = nullptr;
        PlayLayer::onQuit();
    }
};
