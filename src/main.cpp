// Tanzanite — mod menu pour Geometry Dash (Geode 5, GD 2.2081)
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

#include <algorithm>
#include <cstdlib>

using namespace geode::prelude;

// ---------------------------------------------------------------------------
// État du menu (sauvegardé entre les sessions)
// ---------------------------------------------------------------------------
namespace tanzanite {
    struct State {
        bool noclip = false;
        bool speedhack = false;
        float speed = 1.0f;
        bool safeMode = false;
        bool autoSafe = true;      // safe mode forcé quand un cheat est actif
        bool accuratePct = false;
    };

    State s;

    // Couleur bleu-violet de la tanzanite
    const ccColor3B COLOR = {120, 100, 255};

    void load() {
        auto m = Mod::get();
        s.noclip      = m->getSavedValue<bool>("noclip", false);
        s.speedhack   = m->getSavedValue<bool>("speedhack", false);
        s.speed       = m->getSavedValue<float>("speed", 1.0f);
        s.safeMode    = m->getSavedValue<bool>("safe-mode", false);
        s.autoSafe    = m->getSavedValue<bool>("auto-safe", true);
        s.accuratePct = m->getSavedValue<bool>("accurate-pct", false);
    }

    void save() {
        auto m = Mod::get();
        m->setSavedValue("noclip", s.noclip);
        m->setSavedValue("speedhack", s.speedhack);
        m->setSavedValue("speed", s.speed);
        m->setSavedValue("safe-mode", s.safeMode);
        m->setSavedValue("auto-safe", s.autoSafe);
        m->setSavedValue("accurate-pct", s.accuratePct);
    }

    bool cheating() {
        return s.noclip || (s.speedhack && s.speed != 1.0f);
    }

    bool safeActive() {
        return s.safeMode || (s.autoSafe && cheating());
    }
}

$on_mod(Loaded) {
    tanzanite::load();
}

// ---------------------------------------------------------------------------
// Fenêtre du menu
// ---------------------------------------------------------------------------
class TanzaniteMenu : public Popup {
protected:
    enum Tag { NOCLIP = 1, SPEEDHACK, SAFE, AUTOSAFE, ACCURATE };

    CCLabelBMFont* m_status = nullptr;

    void addToggle(CCMenu* menu, char const* label, bool on, int tag, float y) {
        auto toggler = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(TanzaniteMenu::onToggle), 0.7f
        );
        toggler->toggle(on);
        toggler->setTag(tag);
        toggler->setPosition({35.f, y});
        menu->addChild(toggler);

        auto text = CCLabelBMFont::create(label, "bigFont.fnt");
        text->setScale(0.45f);
        text->setAnchorPoint({0.f, 0.5f});
        text->setPosition({55.f, y});
        menu->addChild(text);
    }

    bool init() {
        if (!Popup::init(320.f, 250.f, "GJ_square04.png")) return false;

        this->setTitle("Tanzanite");
        m_title->setColor(tanzanite::COLOR);

        auto size = m_mainLayer->getContentSize();
        auto menu = CCMenu::create();
        menu->setContentSize(size);
        menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(menu);

        auto& s = tanzanite::s;
        float y = size.height - 60.f;
        addToggle(menu, "Noclip", s.noclip, NOCLIP, y);              y -= 32.f;
        addToggle(menu, "Speedhack", s.speedhack, SPEEDHACK, y);

        // Champ de vitesse à droite du speedhack
        auto input = TextInput::create(70.f, "1.0");
        input->setCommonFilter(CommonFilter::Float);
        input->setString(fmt::format("{:.2f}", s.speed).c_str());
        input->setScale(0.8f);
        input->setPosition({size.width - 55.f, y});
        input->setCallback([this](std::string const& str) {
            float v = std::strtof(str.c_str(), nullptr);
            if (v <= 0.f) v = 1.0f;
            tanzanite::s.speed = std::clamp(v, 0.05f, 10.0f);
            tanzanite::save();
            this->refreshStatus();
        });
        m_mainLayer->addChild(input);
        y -= 32.f;

        addToggle(menu, "Safe mode", s.safeMode, SAFE, y);           y -= 32.f;
        addToggle(menu, "Safe mode auto", s.autoSafe, AUTOSAFE, y);  y -= 32.f;
        addToggle(menu, "Pourcentage precis", s.accuratePct, ACCURATE, y);

        m_status = CCLabelBMFont::create("", "chatFont.fnt");
        m_status->setScale(0.7f);
        m_status->setPosition({size.width / 2.f, 18.f});
        m_mainLayer->addChild(m_status);
        this->refreshStatus();

        return true;
    }

    void refreshStatus() {
        if (!m_status) return;
        if (tanzanite::safeActive()) {
            m_status->setString("Safe mode ACTIF : aucune progression sauvegardee");
            m_status->setColor(tanzanite::COLOR);
        } else if (tanzanite::cheating()) {
            m_status->setString("Attention : cheats actifs SANS safe mode");
            m_status->setColor({255, 90, 90});
        } else {
            m_status->setString("Aucun cheat actif");
            m_status->setColor({200, 200, 200});
        }
    }

    void onToggle(CCObject* sender) {
        auto toggler = static_cast<CCMenuItemToggler*>(sender);
        // Le toggler change d'état APRÈS le callback : on lit l'inverse
        bool on = !toggler->isToggled();
        auto& s = tanzanite::s;
        switch (toggler->getTag()) {
            case NOCLIP:    s.noclip = on; break;
            case SPEEDHACK: s.speedhack = on; break;
            case SAFE:      s.safeMode = on; break;
            case AUTOSAFE:  s.autoSafe = on; break;
            case ACCURATE:  s.accuratePct = on; break;
        }
        tanzanite::save();
        this->refreshStatus();
    }

public:
    static TanzaniteMenu* create() {
        auto ret = new TanzaniteMenu();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

static void openTanzanite() {
    if (auto menu = TanzaniteMenu::create()) menu->show();
}

// Bouton « Tanzanite » réutilisé partout
static CCMenuItemSpriteExtra* makeTanzaniteButton(CCObject* target, SEL_MenuHandler sel) {
    auto spr = ButtonSprite::create("Tanzanite", "goldFont.fnt", "GJ_button_02.png", 0.8f);
    spr->setScale(0.6f);
    auto btn = CCMenuItemSpriteExtra::create(spr, target, sel);
    btn->setID("tanzanite-button"_spr);
    return btn;
}

// ---------------------------------------------------------------------------
// Points d'entrée du menu
// ---------------------------------------------------------------------------
class $modify(TanzaniteMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (auto menu = this->getChildByID("bottom-menu")) {
            menu->addChild(makeTanzaniteButton(this, menu_selector(TanzaniteMenuLayer::onTanzanite)));
            menu->updateLayout();
        }
        return true;
    }
    void onTanzanite(CCObject*) { openTanzanite(); }
};

class $modify(TanzanitePauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (auto menu = this->getChildByID("right-button-menu")) {
            menu->addChild(makeTanzaniteButton(this, menu_selector(TanzanitePauseLayer::onTanzanite)));
            menu->updateLayout();
        }
    }
    void onTanzanite(CCObject*) { openTanzanite(); }
};

// ---------------------------------------------------------------------------
// Fonctionnalités
// ---------------------------------------------------------------------------

// Speedhack : on multiplie le temps écoulé de toute la boucle du jeu
class $modify(CCScheduler) {
    void update(float dt) {
        if (tanzanite::s.speedhack) dt *= tanzanite::s.speed;
        CCScheduler::update(dt);
    }
};

class $modify(TanzanitePlayLayer, PlayLayer) {
    // Noclip : on ignore la mort (sauf le pic anti-triche du jeu)
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (tanzanite::s.noclip && object != m_anticheatSpike) return;
        if (tanzanite::safeActive()) m_isTestMode = true;
        PlayLayer::destroyPlayer(player, object);
    }

    // Safe mode : le niveau compte comme un test → rien n'est enregistré
    void levelComplete() {
        if (tanzanite::safeActive()) m_isTestMode = true;
        PlayLayer::levelComplete();
    }

    // Pourcentage avec 2 décimales
    void updateProgressbar() {
        PlayLayer::updateProgressbar();
        if (tanzanite::s.accuratePct && m_percentageLabel) {
            m_percentageLabel->setString(
                fmt::format("{:.2f}%", this->getCurrentPercent()).c_str()
            );
        }
    }
};
