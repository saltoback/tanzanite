// Tanzanite — Simple Editor
// A layer over the normal level editor: GD's many small buttons are hidden and
// replaced by one clear panel (mode, object search, move / rotate / copy, undo,
// playtest). Everything still goes through the editor's own functions, so all
// levels, objects and triggers keep working. One click turns it off.
#include "State.hpp"
#include "Ui.hpp"

#include <Geode/modify/EditorUI.hpp>
#include <Geode/ui/TextInput.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

using namespace geode::prelude;
using namespace tanzanite;
using namespace tanzanite::ui;

namespace {
    // --- Object catalog (names for the search) --------------------------------
    enum Cat { CatBlocks = 1, CatOrbsPads, CatPortals, CatSpeed, CatTriggers, CatRecent };
    constexpr int CAT_COUNT = 7;
    char const* CAT_NAMES[CAT_COUNT] = {"All", "Blocks", "Pads & Orbs", "Portals", "Speed", "Triggers", "Recent"};

    struct EdObj { int id; char const* name; int cat; };
    const std::vector<EdObj> OBJECTS = {
        {1, "Block", CatBlocks}, {2, "Block 2", CatBlocks}, {3, "Block 3", CatBlocks},
        {4, "Block 4", CatBlocks}, {5, "Block 5", CatBlocks}, {6, "Block 6", CatBlocks},
        {7, "Block 7", CatBlocks}, {40, "Slab", CatBlocks}, {8, "Spike", CatBlocks},
        {39, "Small Spike", CatBlocks}, {9, "Ground Spikes", CatBlocks},
        {1329, "User Coin", CatBlocks}, {31, "Start Pos", CatBlocks},

        {35, "Yellow Pad", CatOrbsPads}, {140, "Pink Pad", CatOrbsPads}, {1332, "Red Pad", CatOrbsPads},
        {67, "Blue Pad", CatOrbsPads}, {36, "Yellow Orb", CatOrbsPads}, {141, "Pink Orb", CatOrbsPads},
        {1333, "Red Orb", CatOrbsPads}, {84, "Blue Orb", CatOrbsPads}, {1022, "Green Orb", CatOrbsPads},
        {1330, "Black Orb", CatOrbsPads}, {1704, "Green Dash Orb", CatOrbsPads},
        {1751, "Pink Dash Orb", CatOrbsPads},

        {12, "Cube Portal", CatPortals}, {13, "Ship Portal", CatPortals}, {47, "Ball Portal", CatPortals},
        {111, "UFO Portal", CatPortals}, {660, "Wave Portal", CatPortals}, {745, "Robot Portal", CatPortals},
        {1331, "Spider Portal", CatPortals}, {1933, "Swing Portal", CatPortals},
        {10, "Gravity Down Portal", CatPortals}, {11, "Gravity Up Portal", CatPortals},
        {45, "Mirror Portal", CatPortals}, {46, "Normal Mirror Portal", CatPortals},
        {101, "Mini Portal", CatPortals}, {99, "Normal Size Portal", CatPortals},
        {286, "Dual Portal", CatPortals}, {287, "Single Portal", CatPortals},
        {747, "Teleport Portal", CatPortals},

        {200, "Speed 0.5x (Slow)", CatSpeed}, {201, "Speed 1x (Normal)", CatSpeed},
        {202, "Speed 2x (Fast)", CatSpeed}, {203, "Speed 3x (Faster)", CatSpeed},
        {1334, "Speed 4x (Fastest)", CatSpeed},

        {899, "Color Trigger", CatTriggers}, {901, "Move Trigger", CatTriggers},
        {1006, "Pulse Trigger", CatTriggers}, {1007, "Alpha Trigger", CatTriggers},
        {1049, "Toggle Trigger", CatTriggers}, {1268, "Spawn Trigger", CatTriggers},
        {1346, "Rotate Trigger", CatTriggers}, {1347, "Follow Trigger", CatTriggers},
        {1520, "Shake Trigger", CatTriggers}, {1585, "Animate Trigger", CatTriggers},
        {1595, "Touch Trigger", CatTriggers}, {1611, "Count Trigger", CatTriggers},
        {1616, "Stop Trigger", CatTriggers}, {1811, "Instant Count Trigger", CatTriggers},
        {1815, "Collision Trigger", CatTriggers}, {1817, "Pickup Trigger", CatTriggers},
        {1912, "Random Trigger", CatTriggers}, {1913, "Camera Zoom Trigger", CatTriggers},
        {1914, "Camera Static Trigger", CatTriggers}, {1916, "Camera Offset Trigger", CatTriggers},
        {1934, "Song Trigger", CatTriggers},
    };

    char const* objectName(int id) {
        for (auto const& o : OBJECTS) if (o.id == id) return o.name;
        return nullptr;
    }

    // Last objects picked, newest first (saved)
    std::vector<int> s_recent;
    bool s_recentLoaded = false;

    void loadRecent() {
        if (s_recentLoaded) return;
        s_recentLoaded = true;
        auto text = Mod::get()->getSavedValue<std::string>("editor-recent", "");
        size_t start = 0;
        while (start < text.size()) {
            size_t end = text.find(',', start);
            if (end == std::string::npos) end = text.size();
            int id = std::atoi(text.substr(start, end - start).c_str());
            if (id > 0) s_recent.push_back(id);
            start = end + 1;
        }
    }

    void pushRecent(int id) {
        s_recent.erase(std::remove(s_recent.begin(), s_recent.end(), id), s_recent.end());
        s_recent.insert(s_recent.begin(), id);
        if (s_recent.size() > 15) s_recent.resize(15);
        std::string text;
        for (int r : s_recent) {
            if (!text.empty()) text += ",";
            text += std::to_string(r);
        }
        Mod::get()->setSavedValue("editor-recent", text);
    }

    std::string lower(std::string str) {
        for (auto& c : str) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return str;
    }

    // Icon of an object, sized to fit a square
    CCNode* objectIcon(int id, float size) {
        CCSprite* spr = nullptr;
        if (auto frame = ObjectToolbox::sharedState()->intKeyToFrame(id); frame && *frame) {
            spr = CCSprite::createWithSpriteFrameName(frame);
        }
        if (!spr) spr = CCSprite::createWithSpriteFrameName("square_01_001.png");
        if (!spr) return CCNode::create();
        auto sz = spr->getContentSize();
        float m = std::max(sz.width, sz.height);
        if (m > 0.f) spr->setScale(size / m);
        return spr;
    }

    constexpr float PW = 196.f;   // panel width
    constexpr float PH = 172.f;   // panel height

    // Move steps, from big to tiny
    struct Step { char const* name; EditCommand up, down, left, right; };
    const Step STEPS[] = {
        {"5 Blk", EditCommand::BigUp, EditCommand::BigDown, EditCommand::BigLeft, EditCommand::BigRight},
        {"Block", EditCommand::Up, EditCommand::Down, EditCommand::Left, EditCommand::Right},
        {"Half", EditCommand::HalfUp, EditCommand::HalfDown, EditCommand::HalfLeft, EditCommand::HalfRight},
        {"Small", EditCommand::SmallUp, EditCommand::SmallDown, EditCommand::SmallLeft, EditCommand::SmallRight},
        {"Tiny", EditCommand::TinyUp, EditCommand::TinyDown, EditCommand::TinyLeft, EditCommand::TinyRight},
    };
}

class SimpleEditorPanel : public CCNode {
    EditorUI* m_ui = nullptr;
    CCDrawNode* m_bg = nullptr;
    CCMenu* m_menu = nullptr;          // rebuilt with the layout
    CCNode* m_nodes = nullptr;
    CCMenu* m_gridMenu = nullptr;      // object grid (rebuilt while typing)
    CCNode* m_gridNodes = nullptr;
    CCLabelBMFont* m_info = nullptr;
    TextInput* m_search = nullptr;
    int m_shownMode = -1;
    bool m_dirty = true;
    bool m_gridDirty = false;
    int m_cat = 0;
    int m_page = 0;
    std::string m_query;
    int m_step = 1;
    float m_timer = 0.f;

public:
    static SimpleEditorPanel* create(EditorUI* ui) {
        auto ret = new SimpleEditorPanel();
        if (ret->init(ui)) { ret->autorelease(); return ret; }
        delete ret;
        return nullptr;
    }

    bool init(EditorUI* ui) {
        if (!CCNode::init()) return false;
        m_ui = ui;
        loadRecent();
        this->setID("simple-editor"_spr);
        this->setContentSize({PW, PH});

        // Right side, above the bottom toolbar; smaller on short screens
        auto win = CCDirector::get()->getWinSize();
        float bottom = (ui->m_toolbarHeight > 10.f ? ui->m_toolbarHeight : 90.f) + 4.f;
        float space = win.height - 44.f - bottom;
        float scale = std::min(1.f, space / PH);
        this->setScale(scale);
        this->setAnchorPoint({1.f, 0.f});
        this->ignoreAnchorPointForPosition(false);
        this->setPosition({win.width - 4.f, bottom});

        m_bg = makeDraw();
        drawRoundedRect(m_bg, {0.f, 0.f, PW, PH}, 9.f, PANEL4, 1.3f, PURPLE4);
        this->addChild(m_bg, -1);

        m_nodes = CCNode::create();
        this->addChild(m_nodes);
        m_menu = CCMenu::create();
        m_menu->setContentSize({PW, PH});
        m_menu->ignoreAnchorPointForPosition(false);
        m_menu->setAnchorPoint({0.f, 0.f});
        m_menu->setPosition({0.f, 0.f});
        this->addChild(m_menu);
        m_gridNodes = CCNode::create();
        this->addChild(m_gridNodes);
        m_gridMenu = CCMenu::create();
        m_gridMenu->setContentSize({PW, PH});
        m_gridMenu->ignoreAnchorPointForPosition(false);
        m_gridMenu->setAnchorPoint({0.f, 0.f});
        m_gridMenu->setPosition({0.f, 0.f});
        this->addChild(m_gridMenu);

        this->scheduleUpdate();
        return true;
    }

    // Is a screen point on the panel? (so the editor doesn't place objects there)
    bool contains(CCPoint world) {
        if (!this->isVisible() || !this->getParent()) return false;
        auto p = this->convertToNodeSpace(world);
        return p.x >= -2.f && p.y >= -2.f && p.x <= PW + 2.f && p.y <= PH + 2.f;
    }

    void update(float dt) override {
        if (!this->isVisible()) return;
        int mode = m_ui->m_selectedMode;
        if (mode != m_shownMode || m_dirty) this->rebuild();
        else if (m_gridDirty) this->refreshGrid();

        m_timer += dt;
        if (m_timer > 0.2f && m_info) {
            m_timer = 0.f;
            this->updateInfo();
        }
    }

private:
    // --- Building blocks -------------------------------------------------------
    PillNode* pill(char const* text, float x, float y, float w, std::function<void()> cb, bool selected = false,
                   float h = 16.f, float scale = 0.3f) {
        auto p = PillNode::create(text, w, h, scale);
        p->setSelected(selected);
        auto item = CCMenuItemExt::createSpriteExtra(p, [this, cb](auto) {
            cb();
            m_dirty = true;     // redraw after the click (never inside it)
        });
        item->setPosition({x, y});
        m_menu->addChild(item);
        return p;
    }

    void arrow(float x, float y, float rotation, std::function<void()> cb) {
        auto spr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
        spr->setScale(0.32f);
        spr->setRotation(rotation);
        auto item = CCMenuItemExt::createSpriteExtra(spr, [cb](auto) { cb(); });
        item->setPosition({x, y});
        m_menu->addChild(item);
    }

    void text(char const* str, float x, float y, float scale, ccColor3B color, float anchorX = 0.5f) {
        auto lbl = makeLabel(str, "bigFont.fnt", scale, color);
        lbl->setAnchorPoint({anchorX, 0.5f});
        lbl->setPosition({x, y});
        lbl->limitLabelWidth(PW - 12.f, scale, 0.1f);
        m_nodes->addChild(lbl);
    }

    void setMode(int mode) {
        CCMenuItemSpriteExtra* btn = mode == 1 ? m_ui->m_deleteModeBtn
                                   : mode == 2 ? m_ui->m_buildModeBtn : m_ui->m_editModeBtn;
        if (btn && m_ui->m_selectedMode != mode) m_ui->toggleMode(btn);
    }

    int selectedCount() {
        if (auto arr = m_ui->getSelectedObjects()) return static_cast<int>(arr->count());
        return 0;
    }

    void updateInfo() {
        std::string str;
        switch (m_ui->m_selectedMode) {
            case 2: {
                int id = m_ui->m_selectedObjectIndex;
                auto name = objectName(id);
                str = name ? fmt::format("Placing: {}", name) : fmt::format("Placing: object #{}", id);
                break;
            }
            case 3: {
                int n = selectedCount();
                str = n == 0 ? "Tap an object to select it" : fmt::format("{} selected", n);
                break;
            }
            default: str = "Tap objects to delete them"; break;
        }
        if (std::string(m_info->getString()) != str) {
            m_info->setString(str.c_str());
            m_info->limitLabelWidth(PW - 12.f, 0.42f, 0.1f);
        }
    }

    // --- Layout ------------------------------------------------------------------
    void rebuild() {
        m_dirty = false;
        m_shownMode = m_ui->m_selectedMode;
        m_menu->removeAllChildren();
        m_nodes->removeAllChildren();
        m_gridMenu->removeAllChildren();
        m_gridNodes->removeAllChildren();
        m_search = nullptr;
        m_info = nullptr;

        // Header: the 3 modes
        float y = PH - 12.f;
        int mode = m_ui->m_selectedMode;
        pill("Build", 34.f, y, 58.f, [this] { this->setMode(2); }, mode == 2);
        pill("Edit", 98.f, y, 58.f, [this] { this->setMode(3); }, mode == 3);
        pill("Delete", 162.f, y, 58.f, [this] { this->setMode(1); }, mode == 1);

        if (mode == 2) this->buildLayout();
        else if (mode == 3) this->editLayout();
        else this->deleteLayout();

        // Info line + bottom row: undo / redo / playtest
        m_info = makeLabel("", "chatFont.fnt", 0.42f, {220, 210, 255});
        m_info->setPosition({PW / 2.f, 27.f});
        m_nodes->addChild(m_info);
        this->updateInfo();

        y = 11.f;
        pill("Undo", 28.f, y, 46.f, [this] { m_ui->undoLastAction(nullptr); });
        pill("Redo", 78.f, y, 46.f, [this] { m_ui->redoLastAction(nullptr); });
        pill("Play", 128.f, y, 46.f, [this] { m_ui->onPlaytest(nullptr); });
        pill("Stop", 174.f, y, 38.f, [this] { m_ui->onStopPlaytest(nullptr); });
    }

    void buildLayout() {
        float y = PH - 32.f;
        m_search = TextInput::create(250.f, "Search objects or ID");
        m_search->setScale(0.62f);
        m_search->setPosition({PW / 2.f, y});
        m_search->setString(m_query.c_str());
        m_search->setCallback([this](std::string const& str) {
            m_query = str;
            m_page = 0;
            m_gridDirty = true;
        });
        m_nodes->addChild(m_search);

        y -= 19.f;
        arrow(14.f, y, 180.f, [this] { m_cat = (m_cat + CAT_COUNT - 1) % CAT_COUNT; m_page = 0; m_dirty = true; });
        arrow(PW - 14.f, y, 0.f, [this] { m_cat = (m_cat + 1) % CAT_COUNT; m_page = 0; m_dirty = true; });
        text(CAT_NAMES[m_cat], PW / 2.f, y, 0.34f, PURPLE);

        this->refreshGrid();
    }

    std::vector<int> results() {
        std::vector<int> ids;
        auto q = lower(m_query);
        // A number: that exact object ID first
        bool number = !q.empty() && std::all_of(q.begin(), q.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
        if (number) {
            int id = std::atoi(q.c_str());
            if (id > 0) ids.push_back(id);
        }
        if (m_cat == CatRecent && q.empty()) return s_recent;
        for (auto const& o : OBJECTS) {
            if (m_cat != 0 && m_cat != CatRecent && o.cat != m_cat) continue;
            if (!q.empty() && !number && lower(o.name).find(q) == std::string::npos) continue;
            if (number) continue;
            ids.push_back(o.id);
        }
        return ids;
    }

    void refreshGrid() {
        m_gridDirty = false;
        m_gridMenu->removeAllChildren();
        m_gridNodes->removeAllChildren();

        auto ids = this->results();
        constexpr int COLS = 6, ROWS = 3, PER_PAGE = COLS * ROWS;
        int pages = std::max(1, (int(ids.size()) + PER_PAGE - 1) / PER_PAGE);
        m_page = std::clamp(m_page, 0, pages - 1);

        float cell = 29.f;
        float x0 = (PW - cell * COLS) / 2.f + cell / 2.f;
        float yTop = PH - 74.f;
        int selected = m_ui->m_selectedObjectIndex;
        for (int i = 0; i < PER_PAGE; i++) {
            int idx = m_page * PER_PAGE + i;
            if (idx >= int(ids.size())) break;
            int id = ids[idx];
            float x = x0 + cell * float(i % COLS);
            float y = yTop - cell * float(i / COLS);

            auto box = CCNode::create();
            box->setContentSize({cell - 3.f, cell - 3.f});
            auto bg = makeDraw();
            drawRoundedRect(bg, {0.f, 0.f, cell - 3.f, cell - 3.f}, 5.f,
                id == selected ? rgba(0.59f, 0.43f, 1.f, 0.6f) : rgba(1.f, 1.f, 1.f, 0.06f),
                0.6f, rgba(0.59f, 0.43f, 1.f, id == selected ? 1.f : 0.4f));
            box->addChild(bg);
            auto icon = objectIcon(id, cell - 9.f);
            icon->setPosition({(cell - 3.f) / 2.f, (cell - 3.f) / 2.f});
            box->addChild(icon);

            auto item = CCMenuItemExt::createSpriteExtra(box, [this, id](auto) {
                m_ui->m_selectedObjectIndex = id;
                pushRecent(id);
                this->setMode(2);
                m_gridDirty = true;
            });
            item->setPosition({x, y});
            m_gridMenu->addChild(item);
        }

        if (ids.empty()) {
            auto lbl = makeLabel("Nothing found", "chatFont.fnt", 0.5f, {160, 150, 200});
            lbl->setPosition({PW / 2.f, yTop - cell});
            m_gridNodes->addChild(lbl);
        }

        // Pages
        if (pages > 1) {
            auto lbl = makeLabel(fmt::format("{}/{}", m_page + 1, pages).c_str(), "chatFont.fnt", 0.4f, {170, 160, 210});
            lbl->setPosition({PW / 2.f, yTop - cell * 2.f - 17.f});
            m_gridNodes->addChild(lbl);
            auto prev = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
            prev->setScale(0.22f);
            prev->setFlipX(true);
            auto prevItem = CCMenuItemExt::createSpriteExtra(prev, [this, pages](auto) {
                m_page = (m_page + pages - 1) % pages;
                m_gridDirty = true;
            });
            prevItem->setPosition({PW / 2.f - 26.f, yTop - cell * 2.f - 17.f});
            m_gridMenu->addChild(prevItem);
            auto next = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
            next->setScale(0.22f);
            auto nextItem = CCMenuItemExt::createSpriteExtra(next, [this, pages](auto) {
                m_page = (m_page + 1) % pages;
                m_gridDirty = true;
            });
            nextItem->setPosition({PW / 2.f + 26.f, yTop - cell * 2.f - 17.f});
            m_gridMenu->addChild(nextItem);
        }
    }

    void editLayout() {
        // Move step
        float y = PH - 31.f;
        for (int i = 0; i < 5; i++) {
            pill(STEPS[i].name, 22.f + 38.f * float(i), y, 36.f, [this, i] { m_step = i; }, m_step == i, 14.f, 0.26f);
        }

        // Arrow pad (left) + rotate / flip / copy (right)
        float cx = 40.f, cy = PH - 72.f;
        arrow(cx, cy + 18.f, -90.f, [this] { m_ui->moveObjectCall(STEPS[m_step].up); });
        arrow(cx, cy - 18.f, 90.f, [this] { m_ui->moveObjectCall(STEPS[m_step].down); });
        arrow(cx - 22.f, cy, 180.f, [this] { m_ui->moveObjectCall(STEPS[m_step].left); });
        arrow(cx + 22.f, cy, 0.f, [this] { m_ui->moveObjectCall(STEPS[m_step].right); });
        text("Move", cx, cy, 0.22f, {170, 160, 210});

        float rx = 104.f, rx2 = 158.f;
        y = PH - 52.f;
        pill("Rotate L", rx, y, 50.f, [this] { m_ui->transformObjectCall(EditCommand::RotateCCW); });
        pill("Rotate R", rx2, y, 50.f, [this] { m_ui->transformObjectCall(EditCommand::RotateCW); });
        y -= 19.f;
        pill("Flip X", rx, y, 50.f, [this] { m_ui->transformObjectCall(EditCommand::FlipX); });
        pill("Flip Y", rx2, y, 50.f, [this] { m_ui->transformObjectCall(EditCommand::FlipY); });
        y -= 19.f;
        pill("Copy", rx, y, 50.f, [this] { m_ui->onCopy(nullptr); });
        pill("Paste", rx2, y, 50.f, [this] { m_ui->onPaste(nullptr); });

        // Actions on the selection
        y = PH - 114.f;
        pill("Duplicate", 36.f, y, 62.f, [this] { m_ui->onDuplicate(nullptr); });
        pill("Delete", 98.f, y, 56.f, [this] { m_ui->onDeleteSelected(nullptr); });
        pill("Deselect", 160.f, y, 62.f, [this] { m_ui->onDeselectAll(nullptr); });
        y -= 19.f;
        pill("Settings", 36.f, y, 62.f, [this] { m_ui->editObject(nullptr); });
        pill("Groups", 98.f, y, 56.f, [this] { m_ui->editGroup(nullptr); });
        pill("Swipe", 160.f, y, 62.f, [this] {
            if (m_ui->m_swipeBtn) m_ui->toggleSwipe(m_ui->m_swipeBtn);
        }, m_ui->m_swipeEnabled);
    }

    void deleteLayout() {
        float y = PH - 40.f;
        text("Delete mode", PW / 2.f, y, 0.38f, {255, 255, 255});
        y -= 16.f;
        auto hint = makeLabel("Tap an object to remove it", "chatFont.fnt", 0.5f, {170, 160, 210});
        hint->setPosition({PW / 2.f, y});
        m_nodes->addChild(hint);
        y -= 28.f;
        pill("Delete Selected", PW / 2.f, y, 120.f, [this] { m_ui->onDeleteSelected(nullptr); }, false, 20.f, 0.34f);
        y -= 26.f;
        pill("Back to Edit", PW / 2.f, y, 120.f, [this] { this->setMode(3); }, false, 20.f, 0.34f);
    }
};

// ---------------------------------------------------------------------------
// The editor: add the panel, hide what it replaces
// ---------------------------------------------------------------------------
class $modify(TanzaniteEditorUI, EditorUI) {
    struct Fields {
        SimpleEditorPanel* panel = nullptr;
        CCNode* toggleBtn = nullptr;
        PillNode* togglePill = nullptr;
        std::vector<std::pair<CCNode*, bool>> saved;   // visibility before Simple Mode
        bool hidden = false;
        bool uiShown = true;
    };

    // GD buttons that the panel replaces
    std::vector<CCNode*> replacedNodes() {
        return {
            m_editButtonBar, m_copyBtn, m_pasteBtn, m_copyPasteBtn, m_editObjectBtn, m_editGroupBtn,
            m_editHSVBtn, m_editSpecialBtn, m_copyValuesBtn, m_pasteStateBtn, m_pasteColorBtn,
            m_goToLayerBtn, m_deselectBtn, m_linkBtn, m_unlinkBtn, m_enableLinkBtn, m_layerNextBtn,
            m_layerPrevBtn, m_goToBaseBtn, m_swipeBtn, m_freeMoveBtn, m_snapBtn, m_rotateBtn,
        };
    }

    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        auto win = CCDirector::get()->getWinSize();
        auto panel = SimpleEditorPanel::create(this);
        if (!panel) return true;
        this->addChild(panel, 200);
        m_fields->panel = panel;

        // On / off switch at the top of the screen
        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        menu->setID("simple-editor-toggle"_spr);
        this->addChild(menu, 200);
        auto pillNode = PillNode::create("Simple", 58.f, 18.f, 0.3f);
        auto item = CCMenuItemExt::createSpriteExtra(pillNode, [this](auto) {
            s.simpleEditor = !s.simpleEditor;
            tanzanite::save();
            this->applySimpleMode();
        });
        item->setPosition({win.width / 2.f + 150.f, win.height - 14.f});
        menu->addChild(item);
        m_fields->toggleBtn = menu;
        m_fields->togglePill = pillNode;

        this->applySimpleMode();
        this->schedule(schedule_selector(TanzaniteEditorUI::keepHidden));
        return true;
    }

    void applySimpleMode() {
        auto f = m_fields.operator->();
        bool on = s.simpleEditor;
        if (f->togglePill) f->togglePill->setSelected(on);
        if (f->panel) f->panel->setVisible(on && f->uiShown);

        if (on && !f->hidden) {
            f->saved.clear();
            for (auto n : this->replacedNodes()) {
                if (!n) continue;
                f->saved.push_back({n, n->isVisible()});
                n->setVisible(false);
            }
            f->hidden = true;
        } else if (!on && f->hidden) {
            for (auto& [n, vis] : f->saved) n->setVisible(vis);
            f->saved.clear();
            f->hidden = false;
            // These two depend on the current mode
            if (m_editButtonBar) m_editButtonBar->setVisible(m_selectedMode == 3 && f->uiShown);
            if (m_createButtonBar) m_createButtonBar->setVisible(m_selectedMode == 2 && f->uiShown);
            this->updateButtons();
        }
    }

    // The editor shows some of these again on its own (mode change...)
    void keepHidden(float) {
        if (!s.simpleEditor || !m_fields->hidden) return;
        for (auto n : this->replacedNodes()) if (n && n->isVisible()) n->setVisible(false);
    }

    // Playtest hides the interface: hide the panel too
    void showUI(bool show) {
        EditorUI::showUI(show);
        m_fields->uiShown = show;
        if (m_fields->panel) m_fields->panel->setVisible(show && s.simpleEditor);
        if (m_fields->toggleBtn) m_fields->toggleBtn->setVisible(show);
    }

    // Clicks on the panel never place or select objects behind it
    bool ccTouchBegan(CCTouch* touch, CCEvent* event) {
        if (m_fields->panel && m_fields->panel->contains(touch->getLocation())) return false;
        return EditorUI::ccTouchBegan(touch, event);
    }

    // Typing in the search box must not trigger editor shortcuts
    void keyDown(enumKeyCodes key, double timestamp) {
        if (CCIMEDispatcher::sharedDispatcher()->hasDelegate()) return;
        EditorUI::keyDown(key, timestamp);
    }
};
