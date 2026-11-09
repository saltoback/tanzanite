// Tanzanite — the menu (categories, translucent panel, purple borders, logo)
#include "Menu.hpp"
#include "Bot.hpp"
#include "Hud.hpp"
#include "Ui.hpp"
#include "State.hpp"

#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/Notification.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <vector>

using namespace geode::prelude;
using namespace tanzanite;

using namespace tanzanite::ui;

namespace {
    char const* cornerName(int c) {
        switch (c) {
            case TopLeft: return "Top left";
            case TopRight: return "Top right";
            case BottomLeft: return "Bottom left";
            default: return "Bottom right";
        }
    }
}

// The menu is a bar of categories at the top of the screen. Clicking a
// category unrolls its window right below it; clicking it again rolls it up.
class TanzaniteMenu : public Popup {
    static inline TanzaniteMenu* s_instance = nullptr;

    static constexpr int TABS = 5;
    static constexpr float BAR_H = 24.f;      // height of the category bar
    static constexpr float TAB_W = 64.f;      // width of a category button
    static constexpr float GAP = 6.f;
    static constexpr float WW = 312.f;        // window width
    static constexpr float CX = 14.f;         // left edge of the window content
    static constexpr float CW = 284.f;        // width of the window content
    // Window height for each category
    static constexpr float WH[TABS] = {188.f, 134.f, 236.f, 190.f, 164.f};

    CCSize m_win;
    CCNode* m_bar = nullptr;
    int m_tab = -1;                           // -1: no window open
    float m_top = 0.f;                        // y of the first row in the window
    std::vector<PillNode*> m_tabs;
    std::vector<CCNode*> m_tabItems;
    CCNode* m_window = nullptr;
    CCDrawNode* m_windowBg = nullptr;
    CCMenu* m_content = nullptr;
    CCNode* m_contentNodes = nullptr;
    CCLabelBMFont* m_status = nullptr;
    CCLabelBMFont* m_cornerLbl = nullptr;
    CCLabelBMFont* m_scaleLbl = nullptr;
    CCLabelBMFont* m_opacityLbl = nullptr;
    TextInput* m_speedInput = nullptr;
    int m_macroIndex = 0;
    std::string m_macroName;

    CCMenu* makeMenu(CCSize size) {
        auto menu = CCMenu::create();
        menu->setContentSize(size);
        menu->ignoreAnchorPointForPosition(false);
        menu->setAnchorPoint({0.f, 0.f});
        menu->setPosition({0.f, 0.f});
        return menu;
    }

    bool initMenu() {
        m_win = CCDirector::get()->getWinSize();
        if (!Popup::init(m_win.width, m_win.height)) return false;
        s_instance = this;

        // No GD popup box: only the bar and the windows, over a light dim
        this->setOpacity(70);
        m_bgSprite->setVisible(false);
        m_closeBtn->setVisible(false);
        m_closeBtn->setEnabled(false);

        // --- Category bar ---------------------------------------------------
        float logoW = 112.f;
        float barW = logoW + GAP + TABS * (TAB_W + GAP) + 22.f;
        m_bar = CCNode::create();
        m_bar->setContentSize({barW, BAR_H});
        m_bar->setAnchorPoint({0.5f, 1.f});
        m_bar->ignoreAnchorPointForPosition(false);
        m_bar->setPosition({m_win.width / 2.f, m_win.height - 6.f});
        m_bar->setScale(std::min(1.f, (m_win.width - 12.f) / barW));
        m_mainLayer->addChild(m_bar);

        // Logo block
        auto logoBg = makeDraw();
        drawRoundedRect(logoBg, {0.f, 0.f, logoW, BAR_H}, 7.f, PANEL4, 1.4f, PURPLE4);
        m_bar->addChild(logoBg);
        if (auto logo = CCSprite::create("tanzanite-logo.png"_spr)) {
            logo->setScale(20.f / logo->getContentSize().height);
            logo->setPosition({14.f, BAR_H / 2.f});
            m_bar->addChild(logo);
        }
        auto title = makeLabel("Tanzanite", "bigFont.fnt", 0.38f, PURPLE);
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({28.f, BAR_H / 2.f + 1.f});
        m_bar->addChild(title);
        auto version = makeLabel("v1.6.0", "chatFont.fnt", 0.45f, {170, 160, 210});
        version->setAnchorPoint({1.f, 0.5f});
        version->setPosition({logoW - 5.f, BAR_H / 2.f - 1.f});
        m_bar->addChild(version);

        auto barMenu = makeMenu({barW, BAR_H});
        m_bar->addChild(barMenu);

        // Categories
        char const* names[TABS] = {"Player", "Level", "Labels", "Bot", "Info"};
        float x = logoW + GAP;
        for (int i = 0; i < TABS; i++) {
            auto tab = PillNode::create(names[i], TAB_W, BAR_H, 0.36f);
            auto item = CCMenuItemExt::createSpriteExtra(tab, [this, i](auto) {
                if (m_tab == i) this->closeWindow();
                else this->openTab(i);
            });
            item->setPosition({x + TAB_W / 2.f, BAR_H / 2.f});
            barMenu->addChild(item);
            m_tabs.push_back(tab);
            m_tabItems.push_back(item);
            x += TAB_W + GAP;
        }

        // Close button
        auto closeBtn = CCMenuItemExt::createSpriteExtra(
            PillNode::create("X", 22.f, BAR_H, 0.4f),
            [this](auto) { this->onClose(nullptr); }
        );
        closeBtn->setPosition({x + 11.f, BAR_H / 2.f});
        barMenu->addChild(closeBtn);

        auto hint = makeLabel(CLOSE_HINT, "chatFont.fnt", 0.5f, {200, 190, 235});
        hint->setPosition({m_win.width / 2.f, 10.f});
        hint->setOpacity(170);
        m_mainLayer->addChild(hint);

        // --- The window (shown under the selected category) ------------------
        m_window = CCNode::create();
        m_window->setAnchorPoint({0.f, 1.f});
        m_window->ignoreAnchorPointForPosition(false);
        m_window->setVisible(false);
        m_mainLayer->addChild(m_window);
        m_windowBg = makeDraw();
        m_window->addChild(m_windowBg, -1);
        m_content = makeMenu({WW, 300.f});
        m_window->addChild(m_content);
        m_contentNodes = CCNode::create();
        m_window->addChild(m_contentNodes);
        m_status = makeLabel("", "chatFont.fnt", 0.5f);
        m_status->setPosition({WW / 2.f, 11.f});
        m_window->addChild(m_status);

        // Only the categories at first
        return true;
    }

    // Unroll the window of a category, right below its button
    void openTab(int tab) {
        float h = WH[tab];
        m_window->setContentSize({WW, h});
        m_windowBg->clear();
        drawRoundedRect(m_windowBg, {0.f, 0.f, WW, h}, 10.f, PANEL4, 1.4f, PURPLE4);
        m_windowBg->drawSegment({10.f, 22.f}, {WW - 10.f, 22.f}, 0.4f, rgba(0.59f, 0.43f, 1.f, 0.35f));

        // Left edge under the category button, kept on screen
        auto item = m_tabItems[tab];
        auto world = m_bar->convertToWorldSpace({item->getPositionX() - TAB_W / 2.f, 0.f});
        auto local = m_mainLayer->convertToNodeSpace(world);
        float x = std::clamp(local.x, 6.f, m_win.width - WW - 6.f);
        m_window->setPosition({x, local.y - 6.f});

        this->buildTab(tab);

        m_window->stopAllActions();
        m_window->setVisible(true);
        m_window->setScaleY(0.f);
        m_window->runAction(CCEaseBackOut::create(CCScaleTo::create(0.22f, 1.f, 1.f)));
    }

    // Roll the window back up
    void closeWindow() {
        if (m_tab < 0) return;
        m_tab = -1;
        for (auto t : m_tabs) t->setSelected(false);
        m_window->stopAllActions();
        m_window->runAction(CCSequence::create(
            CCEaseIn::create(CCScaleTo::create(0.14f, 1.f, 0.f), 2.f),
            CallFuncExt::create([this] {
                m_window->setVisible(false);
                m_content->removeAllChildren();
                m_contentNodes->removeAllChildren();
                m_cornerLbl = m_scaleLbl = m_opacityLbl = nullptr;
                m_speedInput = nullptr;
            }),
            nullptr
        ));
    }

    // --- Building blocks ----------------------------------------------------

    void addText(char const* text, float x, float y, char const* font, float scale, ccColor3B color, float anchorX = 0.f) {
        auto lbl = makeLabel(text, font, scale, color);
        lbl->setAnchorPoint({anchorX, 0.5f});
        lbl->setPosition({x, y});
        m_contentNodes->addChild(lbl);
    }

    void addSwitch(char const* text, bool* value, float x, float y, float w, float textScale = 0.38f,
                   std::function<void(bool)> onChange = nullptr) {
        auto lbl = makeLabel(text, "bigFont.fnt", textScale);
        lbl->setAnchorPoint({0.f, 0.5f});
        lbl->setPosition({x, y});
        lbl->limitLabelWidth(w - 42.f, textScale, 0.1f);
        m_contentNodes->addChild(lbl);

        auto sw = SwitchNode::create(*value);
        auto item = CCMenuItemExt::createSpriteExtra(sw, [this, value, sw, onChange](auto) {
            *value = !*value;
            sw->setOn(*value);
            if (onChange) onChange(*value);
            tanzanite::save();
            this->refreshStatus();
        });
        item->setPosition({x + w - 18.f, y});
        m_content->addChild(item);
    }

    void addSmallButton(char const* text, float x, float y, std::function<void()> cb, float scale = 0.45f) {
        auto item = CCMenuItemExt::createSpriteExtra(
            makeLabel(text, "bigFont.fnt", scale, PURPLE),
            [cb](auto) { cb(); }
        );
        item->setPosition({x, y});
        m_content->addChild(item);
    }

    CCLabelBMFont* addStepper(char const* text, float x, float y, float valueX,
                              std::function<void()> minus, std::function<void()> plus) {
        addText(text, x, y, "bigFont.fnt", 0.36f, {255, 255, 255});
        addSmallButton("<", valueX - 42.f, y, std::move(minus));
        addSmallButton(">", valueX + 42.f, y, std::move(plus));
        auto val = makeLabel("", "bigFont.fnt", 0.34f, {220, 210, 255});
        val->setPosition({valueX, y});
        m_contentNodes->addChild(val);
        return val;
    }

    // --- Tabs ---------------------------------------------------------------

    // Fill the window with a category (also used to refresh it in place)
    void buildTab(int tab) {
        m_tab = tab;
        m_top = WH[tab] - 18.f;
        m_content->removeAllChildren();
        m_contentNodes->removeAllChildren();
        m_cornerLbl = m_scaleLbl = m_opacityLbl = nullptr;
        m_speedInput = nullptr;
        for (int i = 0; i < int(m_tabs.size()); i++) m_tabs[i]->setSelected(i == tab);

        switch (tab) {
            case 0: buildPlayer(); break;
            case 1: buildLevel(); break;
            case 2: buildLabels(); break;
            case 3: buildBot(); break;
            default: buildInfo(); break;
        }
        this->refreshStatus();
    }

    void buildPlayer() {
        float y = m_top;
        addSwitch("Noclip", &s.noclip, CX, y, CW);
        addText("Go through obstacles without dying", CX, y - 13.f, "chatFont.fnt", 0.5f, {160, 150, 200});
        y -= 40.f;
        addSwitch("Speedhack", &s.speedhack, CX, y, CW);
        addText("Speeds up or slows down the game (not the music)", CX, y - 13.f, "chatFont.fnt", 0.5f, {160, 150, 200});
        y -= 40.f;

        addText("Speed", CX, y, "bigFont.fnt", 0.38f, {255, 255, 255});
        m_speedInput = TextInput::create(70.f, "1.0");
        m_speedInput->setCommonFilter(CommonFilter::Float);
        m_speedInput->setString(fmt::format("{:.2f}", s.speed).c_str());
        m_speedInput->setScale(0.75f);
        m_speedInput->setPosition({CX + 98.f, y});
        m_speedInput->setCallback([this](std::string const& str) {
            float v = std::strtof(str.c_str(), nullptr);
            if (v <= 0.f) v = 1.0f;
            s.speed = std::clamp(v, 0.05f, 10.0f);
            tanzanite::save();
            this->refreshStatus();
        });
        m_contentNodes->addChild(m_speedInput);

        float presets[] = {0.5f, 1.f, 1.5f, 2.f};
        for (int i = 0; i < 4; i++) {
            float p = presets[i];
            addSmallButton(fmt::format("{}x", p).c_str(), CX + 160.f + 33.f * float(i), y, [this, p]() {
                s.speed = p;
                if (m_speedInput) m_speedInput->setString(fmt::format("{:.2f}", p).c_str());
                tanzanite::save();
                this->refreshStatus();
            }, 0.35f);
        }
        y -= 28.f;

        addSwitch("Show Hitboxes", &s.showHitboxes, CX, y, CW);
        y -= 26.f;
        addSwitch("Show Trajectory", &s.showTrajectory, CX, y, CW);
        addText("Path if you hold (green) or release (red)", CX, y - 13.f, "chatFont.fnt", 0.5f, {160, 150, 200});
    }

    void buildLevel() {
        float y = m_top;
        addSwitch("Safe Mode", &s.safeMode, CX, y, CW);
        addText("The level counts as a test run: no progress is saved", CX, y - 13.f, "chatFont.fnt", 0.5f, {160, 150, 200});
        y -= 40.f;
        addSwitch("Auto Safe Mode", &s.autoSafe, CX, y, CW);
        addText("Turns on Safe Mode for any attempt where a cheat was used", CX, y - 13.f, "chatFont.fnt", 0.5f, {160, 150, 200});
        y -= 40.f;
        addSwitch("Accurate Percentage", &s.accuratePct, CX, y, CW);
        addText("Shows the percentage with 2 decimals", CX, y - 13.f, "chatFont.fnt", 0.5f, {160, 150, 200});
    }

    void buildLabels() {
        struct Entry { char const* name; bool* value; };
        Entry entries[] = {
            {"Cheat Indicator", &s.lblCheat},
            {"FPS", &s.lblFps},
            {"CPS", &s.lblCps},
            {"Noclip Accuracy", &s.lblNoclipAcc},
            {"Noclip Deaths", &s.lblNoclipDeaths},
            {"Attempts", &s.lblAttempts},
            {"Jumps", &s.lblJumps},
            {"Level Time", &s.lblTime},
            {"Clock", &s.lblClock},
            {"Best Run", &s.lblBestRun},
            {"Custom Message", &s.lblMessage},
        };
        float colW = (CW - 8.f) / 2.f;
        for (int i = 0; i < int(std::size(entries)); i++) {
            float x = CX + (i % 2) * (colW + 8.f);
            float y = m_top - 21.f * float(i / 2);
            addSwitch(entries[i].name, entries[i].value, x, y, colW, 0.32f);
        }

        float y = m_top - 21.f * 6.f - 6.f;   // below the grid
        m_cornerLbl = addStepper("Position", CX, y, CX + 150.f,
            [this]() { s.corner = (s.corner + 3) % 4; this->onLabelSettingChanged(); },
            [this]() { s.corner = (s.corner + 1) % 4; this->onLabelSettingChanged(); });
        y -= 22.f;
        m_scaleLbl = addStepper("Size", CX, y, CX + 88.f,
            [this]() { s.labelScale = std::max(0.4f, s.labelScale - 0.1f); this->onLabelSettingChanged(); },
            [this]() { s.labelScale = std::min(2.5f, s.labelScale + 0.1f); this->onLabelSettingChanged(); });
        m_opacityLbl = addStepper("", CX + 142.f, y, CX + 230.f,
            [this]() { s.labelOpacity = std::max(30, s.labelOpacity - 25); this->onLabelSettingChanged(); },
            [this]() { s.labelOpacity = std::min(255, s.labelOpacity + 25); this->onLabelSettingChanged(); });
        addText("Opacity", CX + 142.f, y, "bigFont.fnt", 0.3f, {255, 255, 255});
        y -= 22.f;

        addText("Message", CX, y, "bigFont.fnt", 0.36f, {255, 255, 255});
        auto input = TextInput::create(220.f, "Your message");
        input->setString(s.message.c_str());
        input->setScale(0.7f);
        input->setPosition({CX + 172.f, y});
        input->setCallback([](std::string const& str) {
            s.message = str;
            tanzanite::save();
        });
        m_contentNodes->addChild(input);

        this->onLabelSettingChanged();
    }

    void onLabelSettingChanged() {
        if (m_cornerLbl) m_cornerLbl->setString(cornerName(s.corner));
        if (m_scaleLbl) m_scaleLbl->setString(fmt::format("{:.1f}", s.labelScale).c_str());
        if (m_opacityLbl) m_opacityLbl->setString(fmt::format("{}%", s.labelOpacity * 100 / 255).c_str());
        tanzanite::save();
    }

    // Pill button that runs an action
    void addPill(char const* text, float x, float y, float w, bool selected, std::function<void()> cb) {
        auto pill = PillNode::create(text, w, 18.f, 0.32f);
        pill->setSelected(selected);
        auto item = CCMenuItemExt::createSpriteExtra(pill, [cb](auto) { cb(); });
        item->setPosition({x, y});
        m_content->addChild(item);
    }

    void buildBot() {
        using bot::Mode;
        float y = m_top;

        // One big button: everything else happens in the level itself
        auto bigBtn = PillNode::create(bot::macroMode ? "Exit Macro Mode" : "Enter Macro Mode", 170.f, 24.f, 0.42f);
        bigBtn->setSelected(!bot::macroMode);
        auto bigItem = CCMenuItemExt::createSpriteExtra(bigBtn, [this](auto) {
            if (bot::macroMode) {
                hud::exitMacroMode();
                this->buildTab(3);
            } else {
                hud::enterMacroMode();
                // In a level: close the menu, the macro interface is on screen
                if (PlayLayer::get()) this->onClose(nullptr);
                else this->buildTab(3);
            }
        });
        bigItem->setPosition({CX + 88.f, y - 2.f});
        m_content->addChild(bigItem);
        y -= 22.f;

        char const* hint = bot::macroMode
            ? "Macro mode is on: all the controls are at the bottom of the level"
            : "Opens a full macro interface in the level (practice mode is automatic)";
        addText(hint, CX, y, "chatFont.fnt", 0.48f, {160, 150, 200});
        y -= 14.f;
        addText(bot::macroInfo().c_str(), CX, y, "chatFont.fnt", 0.55f, {220, 210, 255});
        y -= 22.f;

        // Saved macros: browse and load
        auto files = bot::listMacros();
        if (m_macroIndex >= int(files.size())) m_macroIndex = 0;
        addText("Saved", CX, y, "bigFont.fnt", 0.36f, {255, 255, 255});
        std::string current = files.empty() ? "No saved macros" : files[m_macroIndex].stem().string();
        auto fileLbl = makeLabel(current.c_str(), "chatFont.fnt", 0.55f, {220, 210, 255});
        fileLbl->limitLabelWidth(120.f, 0.55f, 0.1f);
        fileLbl->setPosition({CX + 130.f, y});
        m_contentNodes->addChild(fileLbl);
        if (!files.empty()) {
            int count = int(files.size());
            addSmallButton("<", CX + 58.f, y, [this, count]() { m_macroIndex = (m_macroIndex + count - 1) % count; this->buildTab(3); }, 0.4f);
            addSmallButton(">", CX + 202.f, y, [this, count]() { m_macroIndex = (m_macroIndex + 1) % count; this->buildTab(3); }, 0.4f);
            auto path = files[m_macroIndex];
            addPill("Load", CX + CW - 26.f, y, 48.f, false, [this, path]() {
                if (bot::loadMacro(path)) {
                    Notification::create("Macro loaded: " + bot::macro.name, NotificationIcon::Success)->show();
                } else {
                    Notification::create("Could not load this macro", NotificationIcon::Error)->show();
                }
                this->buildTab(3);
            });
        }
        y -= 24.f;

        // Save the current macro under a name, or clear it
        if (m_macroName.empty()) m_macroName = bot::macro.name;
        auto nameInput = TextInput::create(170.f, "Macro name");
        nameInput->setString(m_macroName.c_str());
        nameInput->setScale(0.65f);
        nameInput->setPosition({CX + 56.f, y});
        nameInput->setCallback([this](std::string const& str) { m_macroName = str; });
        m_contentNodes->addChild(nameInput);
        addPill("Save", CX + 152.f, y, 48.f, false, [this]() {
            if (bot::macro.inputs.empty()) {
                Notification::create("Nothing to save yet", NotificationIcon::Warning)->show();
            } else if (bot::saveMacro(m_macroName)) {
                Notification::create("Macro saved: " + bot::macro.name, NotificationIcon::Success)->show();
            } else {
                Notification::create("Could not save the macro", NotificationIcon::Error)->show();
            }
            this->buildTab(3);
        });
        addPill("Clear", CX + 206.f, y, 48.f, false, [this]() {
            bot::macro.clear();
            this->buildTab(3);
        });
        y -= 26.f;

        addSwitch("Position Fix", &bot::positionFix, CX, y, CW, 0.36f);
        addText("Restores the exact player state at every input: no desync", CX, y - 12.f, "chatFont.fnt", 0.45f, {160, 150, 200});
        y -= 30.f;
        addSwitch("Show Trajectory", &s.showTrajectory, CX, y, CW, 0.36f);
        addText("Path if you hold (green) or release (red)", CX, y - 12.f, "chatFont.fnt", 0.45f, {160, 150, 200});
    }

    void buildInfo() {
        float y = m_top;
        addText("Tanzanite v1.6.0", CX, y, "bigFont.fnt", 0.45f, PURPLE);
        y -= 24.f;
        char const* lines[] = {
            OPEN_HINT,
            "Auto Safe Mode keeps your progress clean when you cheat",
            "Labels: shown in levels, pick any screen corner",
            "Bot: Enter Macro Mode, then everything is in the level",
            "Open source mod under the MIT license",
            "github.com/saltoback/tanzanite",
        };
        for (auto line : lines) {
            addText(line, CX, y, "chatFont.fnt", 0.6f, {210, 200, 240});
            y -= 20.f;
        }
    }

    void refreshStatus() {
        if (!m_status) return;
        if (tanzanite::safeActive()) {
            m_status->setString("Safe Mode ON: no progress will be saved");
            m_status->setColor(PURPLE);
        } else if (tanzanite::cheating()) {
            m_status->setString("Warning: cheats are on WITHOUT Safe Mode");
            m_status->setColor({255, 90, 90});
        } else {
            m_status->setString("No cheats active");
            m_status->setColor({190, 190, 200});
        }
    }

public:
    ~TanzaniteMenu() {
        if (s_instance == this) s_instance = nullptr;
    }

    void onClose(CCObject* sender) override {
        if (s_instance == this) s_instance = nullptr;
        Popup::onClose(sender);
    }

    static TanzaniteMenu* create() {
        auto ret = new TanzaniteMenu();
        if (ret->initMenu()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    static void toggle() {
        if (s_instance) {
            s_instance->onClose(nullptr);
            return;
        }
        if (auto menu = TanzaniteMenu::create()) {
            menu->show();
            tanzanite::showCursor();
        }
    }
};

void tanzanite::toggleMenu() {
    TanzaniteMenu::toggle();
}
