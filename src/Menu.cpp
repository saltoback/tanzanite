// Tanzanite — the menu (categories, translucent panel, purple borders, logo)
#include "Menu.hpp"
#include "State.hpp"

#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <vector>

using namespace geode::prelude;
using namespace tanzanite;

namespace {
    constexpr float PI = 3.14159265f;

    ccColor4F rgba(float r, float g, float b, float a) { return {r, g, b, a}; }

    // CCDrawNode blends as premultiplied alpha by default, which turns
    // translucent colors opaque. Use regular alpha blending instead.
    CCDrawNode* makeDraw() {
        auto dn = CCDrawNode::create();
        dn->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
        return dn;
    }

    // Rounded rectangle
    void drawRoundedRect(CCDrawNode* dn, CCRect r, float rad, ccColor4F fill, float bw, ccColor4F border) {
        std::vector<CCPoint> pts;
        constexpr int SEG = 6;
        auto corner = [&](float cx, float cy, float a0) {
            for (int i = 0; i <= SEG; i++) {
                float a = a0 + (PI / 2.f) * float(i) / float(SEG);
                CCPoint p = {cx + rad * std::cos(a), cy + rad * std::sin(a)};
                // Skip duplicate points: they create spikes in the border
                if (!pts.empty() && pts.back().getDistance(p) < 0.01f) continue;
                pts.push_back(p);
            }
        };
        corner(r.getMaxX() - rad, r.getMaxY() - rad, 0.f);
        corner(r.getMinX() + rad, r.getMaxY() - rad, PI / 2.f);
        corner(r.getMinX() + rad, r.getMinY() + rad, PI);
        corner(r.getMaxX() - rad, r.getMinY() + rad, 3.f * PI / 2.f);
        if (pts.size() > 1 && pts.back().getDistance(pts.front()) < 0.01f) pts.pop_back();
        dn->drawPolygon(pts.data(), static_cast<unsigned>(pts.size()), fill, bw, border);
    }

    CCLabelBMFont* makeLabel(char const* text, char const* font, float scale, ccColor3B color = {255, 255, 255}) {
        auto lbl = CCLabelBMFont::create(text, font);
        lbl->setScale(scale);
        lbl->setColor(color);
        return lbl;
    }

    // Rounded on/off switch
    class SwitchNode : public CCNode {
        CCDrawNode* m_draw = nullptr;
    public:
        static SwitchNode* create(bool on) {
            auto ret = new SwitchNode();
            if (ret->init(on)) { ret->autorelease(); return ret; }
            delete ret;
            return nullptr;
        }
        bool init(bool on) {
            if (!CCNode::init()) return false;
            this->setContentSize({30.f, 16.f});
            this->setAnchorPoint({0.5f, 0.5f});
            m_draw = makeDraw();
            this->addChild(m_draw);
            this->setOn(on);
            return true;
        }
        void setOn(bool on) {
            m_draw->clear();
            drawRoundedRect(
                m_draw, {0.f, 0.f, 30.f, 16.f}, 8.f,
                on ? PURPLE4 : rgba(0.22f, 0.22f, 0.28f, 0.9f), 1.f,
                on ? rgba(0.82f, 0.74f, 1.f, 1.f) : rgba(0.42f, 0.42f, 0.5f, 1.f)
            );
            m_draw->drawCircle(
                on ? CCPoint{22.f, 8.f} : CCPoint{8.f, 8.f}, 5.5f,
                rgba(1.f, 1.f, 1.f, 1.f), 0.f, rgba(0.f, 0.f, 0.f, 0.f), 24
            );
        }
    };

    // Category tab in the sidebar
    class TabNode : public CCNode {
        CCDrawNode* m_draw = nullptr;
        CCLabelBMFont* m_label = nullptr;
    public:
        static TabNode* create(char const* text) {
            auto ret = new TabNode();
            if (ret->init(text)) { ret->autorelease(); return ret; }
            delete ret;
            return nullptr;
        }
        bool init(char const* text) {
            if (!CCNode::init()) return false;
            this->setContentSize({86.f, 24.f});
            this->setAnchorPoint({0.5f, 0.5f});
            m_draw = makeDraw();
            this->addChild(m_draw);
            m_label = makeLabel(text, "bigFont.fnt", 0.4f);
            m_label->setPosition({43.f, 12.f});
            this->addChild(m_label);
            this->setSelected(false);
            return true;
        }
        void setSelected(bool sel) {
            m_draw->clear();
            if (sel) {
                drawRoundedRect(m_draw, {0.f, 0.f, 86.f, 24.f}, 6.f,
                    rgba(0.59f, 0.43f, 1.f, 0.85f), 0.8f, rgba(0.85f, 0.78f, 1.f, 1.f));
                m_label->setColor({255, 255, 255});
            } else {
                drawRoundedRect(m_draw, {0.f, 0.f, 86.f, 24.f}, 6.f,
                    rgba(1.f, 1.f, 1.f, 0.04f), 0.6f, rgba(0.59f, 0.43f, 1.f, 0.35f));
                m_label->setColor({190, 180, 225});
            }
        }
    };

    char const* cornerName(int c) {
        switch (c) {
            case TopLeft: return "Top left";
            case TopRight: return "Top right";
            case BottomLeft: return "Bottom left";
            default: return "Bottom right";
        }
    }
}

class TanzaniteMenu : public Popup {
    static inline TanzaniteMenu* s_instance = nullptr;

    static constexpr float W = 420.f;
    static constexpr float H = 270.f;
    static constexpr float CX = 124.f;    // left edge of the content area
    static constexpr float CW = 284.f;    // width of the content area

    int m_tab = 0;
    std::vector<TabNode*> m_tabs;
    CCMenu* m_content = nullptr;
    CCNode* m_contentNodes = nullptr;
    CCLabelBMFont* m_status = nullptr;
    CCLabelBMFont* m_cornerLbl = nullptr;
    CCLabelBMFont* m_scaleLbl = nullptr;
    CCLabelBMFont* m_opacityLbl = nullptr;
    TextInput* m_speedInput = nullptr;

    CCMenu* makeMenu() {
        auto menu = CCMenu::create();
        menu->setContentSize({W, H});
        menu->ignoreAnchorPointForPosition(false);
        menu->setAnchorPoint({0.f, 0.f});
        menu->setPosition({0.f, 0.f});
        return menu;
    }

    bool initMenu() {
        if (!Popup::init(W, H)) return false;
        s_instance = this;

        // Barely dim the screen and hide the default GD popup background
        this->setOpacity(60);
        m_bgSprite->setVisible(false);
        m_closeBtn->setVisible(false);
        m_closeBtn->setEnabled(false);

        // Translucent panel with a purple border
        auto bg = makeDraw();
        m_mainLayer->addChild(bg, -1);
        drawRoundedRect(bg, {0.f, 0.f, W, H}, 12.f, PANEL4, 1.6f, PURPLE4);
        drawRoundedRect(bg, {10.f, 30.f, 98.f, H - 82.f}, 8.f, SIDEBAR4, 0.8f, rgba(0.59f, 0.43f, 1.f, 0.45f));
        drawRoundedRect(bg, {CX - 6.f, 30.f, CW + 8.f, H - 82.f}, 8.f, rgba(1.f, 1.f, 1.f, 0.03f), 0.8f, rgba(0.59f, 0.43f, 1.f, 0.30f));
        bg->drawSegment({14.f, H - 44.f}, {W - 14.f, H - 44.f}, 0.7f, rgba(0.59f, 0.43f, 1.f, 0.7f));

        // Header: logo + title. The logo is a plain file in the mod resources
        // (Geode returns a placeholder frame instead of null for unknown frames).
        if (auto logo = CCSprite::create("tanzanite-logo.png"_spr)) {
            logo->setScale(30.f / logo->getContentSize().height);
            logo->setPosition({28.f, H - 22.f});
            m_mainLayer->addChild(logo);
        }
        auto title = makeLabel("Tanzanite", "bigFont.fnt", 0.6f, PURPLE);
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({48.f, H - 21.f});
        m_mainLayer->addChild(title);

        auto version = makeLabel("v1.2.0", "chatFont.fnt", 0.6f, {170, 160, 210});
        version->setAnchorPoint({0.f, 0.5f});
        version->setPosition({48.f + title->getScaledContentSize().width + 6.f, H - 24.f});
        m_mainLayer->addChild(version);

        auto hint = makeLabel("Press the key above Tab to close", "chatFont.fnt", 0.55f, {150, 140, 190});
        hint->setAnchorPoint({1.f, 0.5f});
        hint->setPosition({W - 40.f, H - 23.f});
        m_mainLayer->addChild(hint);

        auto header = makeMenu();
        m_mainLayer->addChild(header);
        auto closeBtn = CCMenuItemExt::createSpriteExtra(
            makeLabel("X", "bigFont.fnt", 0.5f, PURPLE),
            [this](auto) { this->onClose(nullptr); }
        );
        closeBtn->setPosition({W - 20.f, H - 22.f});
        header->addChild(closeBtn);

        // Categories
        char const* names[] = {"Player", "Level", "Labels", "Info"};
        for (int i = 0; i < 4; i++) {
            auto tab = TabNode::create(names[i]);
            auto item = CCMenuItemExt::createSpriteExtra(tab, [this, i](auto) { this->buildTab(i); });
            item->setPosition({59.f, H - 66.f - 30.f * float(i)});
            header->addChild(item);
            m_tabs.push_back(tab);
        }

        // Content area (rebuilt on every tab change)
        m_content = makeMenu();
        m_mainLayer->addChild(m_content);
        m_contentNodes = CCNode::create();
        m_mainLayer->addChild(m_contentNodes);

        m_status = makeLabel("", "chatFont.fnt", 0.65f);
        m_status->setPosition({W / 2.f, 16.f});
        m_mainLayer->addChild(m_status);

        this->buildTab(0);
        return true;
    }

    // --- Building blocks ----------------------------------------------------

    void addText(char const* text, float x, float y, char const* font, float scale, ccColor3B color, float anchorX = 0.f) {
        auto lbl = makeLabel(text, font, scale, color);
        lbl->setAnchorPoint({anchorX, 0.5f});
        lbl->setPosition({x, y});
        m_contentNodes->addChild(lbl);
    }

    void addSwitch(char const* text, bool* value, float x, float y, float w, float textScale = 0.38f) {
        auto lbl = makeLabel(text, "bigFont.fnt", textScale);
        lbl->setAnchorPoint({0.f, 0.5f});
        lbl->setPosition({x, y});
        lbl->limitLabelWidth(w - 42.f, textScale, 0.1f);
        m_contentNodes->addChild(lbl);

        auto sw = SwitchNode::create(*value);
        auto item = CCMenuItemExt::createSpriteExtra(sw, [this, value, sw](auto) {
            *value = !*value;
            sw->setOn(*value);
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

    void buildTab(int tab) {
        m_tab = tab;
        m_content->removeAllChildren();
        m_contentNodes->removeAllChildren();
        m_cornerLbl = m_scaleLbl = m_opacityLbl = nullptr;
        m_speedInput = nullptr;
        for (int i = 0; i < int(m_tabs.size()); i++) m_tabs[i]->setSelected(i == tab);

        switch (tab) {
            case 0: buildPlayer(); break;
            case 1: buildLevel(); break;
            case 2: buildLabels(); break;
            default: buildInfo(); break;
        }
        this->refreshStatus();
    }

    void buildPlayer() {
        float y = H - 66.f;
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
    }

    void buildLevel() {
        float y = H - 66.f;
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
            {"Custom Message", &s.lblMessage},
        };
        float colW = (CW - 8.f) / 2.f;
        for (int i = 0; i < 10; i++) {
            float x = CX + (i % 2) * (colW + 8.f);
            float y = H - 64.f - 21.f * float(i / 2);
            addSwitch(entries[i].name, entries[i].value, x, y, colW, 0.32f);
        }

        float y = H - 64.f - 21.f * 5.f - 6.f;   // below the grid
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

    void buildInfo() {
        float y = H - 66.f;
        addText("Tanzanite v1.2.0", CX, y, "bigFont.fnt", 0.45f, PURPLE);
        y -= 24.f;
        char const* lines[] = {
            "Open / close the menu: key above Tab (Escape also closes it)",
            "Auto Safe Mode keeps your progress clean when you cheat",
            "Labels: shown in levels, pick any screen corner",
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
            PlatformToolbox::showCursor();
        }
    }
};

void tanzanite::toggleMenu() {
    TanzaniteMenu::toggle();
}
