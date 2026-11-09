// Tanzanite — floating menu button, mobile only (there is no ² key on a phone)
// Tap it to open / close the menu, drag it to move it anywhere on the screen.
#include <Geode/Geode.hpp>

#ifdef GEODE_IS_MOBILE

#include "Menu.hpp"

#include <algorithm>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/ui/OverlayManager.hpp>

using namespace geode::prelude;

namespace {
    constexpr float SIZE = 34.f;          // button size on screen
    constexpr float DRAG_START = 8.f;     // finger movement before it counts as a drag

    class MobileButton : public CCNode, public CCTouchDelegate {
        CCSprite* m_sprite = nullptr;
        CCPoint m_touchStart;
        bool m_dragging = false;

    public:
        static MobileButton* create() {
            auto ret = new MobileButton();
            if (ret->init()) { ret->autorelease(); return ret; }
            delete ret;
            return nullptr;
        }

        bool init() override {
            if (!CCNode::init()) return false;
            this->setID("mobile-button"_spr);
            this->setZOrder(10000);

            m_sprite = CCSprite::create("tanzanite-logo.png"_spr);
            if (!m_sprite) m_sprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
            m_sprite->setScale(SIZE / m_sprite->getContentSize().height);
            m_sprite->setOpacity(210);
            this->addChild(m_sprite);

            // Saved position, or the right edge of the screen by default
            auto win = CCDirector::get()->getWinSize();
            float x = Mod::get()->getSavedValue<float>("mobile-button-x", win.width - 26.f);
            float y = Mod::get()->getSavedValue<float>("mobile-button-y", win.height / 2.f + 50.f);
            m_sprite->setPosition(clampToScreen({x, y}));

            // Very high priority so the game never steals the tap
            CCTouchDispatcher::get()->addTargetedDelegate(this, -600, true);
            return true;
        }

        CCPoint clampToScreen(CCPoint p) {
            auto win = CCDirector::get()->getWinSize();
            float h = SIZE / 2.f;
            return {std::clamp(p.x, h, win.width - h), std::clamp(p.y, h, win.height - h)};
        }

        bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
            if (!this->isVisible()) return false;
            auto p = touch->getLocation();
            if (p.getDistance(m_sprite->getPosition()) > SIZE * 0.6f) return false;
            m_touchStart = p;
            m_dragging = false;
            m_sprite->setScale(SIZE * 1.1f / m_sprite->getContentSize().height);
            return true;
        }

        void ccTouchMoved(CCTouch* touch, CCEvent*) override {
            auto p = touch->getLocation();
            if (!m_dragging && p.getDistance(m_touchStart) > DRAG_START) m_dragging = true;
            if (m_dragging) m_sprite->setPosition(clampToScreen(p));
        }

        void ccTouchEnded(CCTouch*, CCEvent*) override {
            m_sprite->setScale(SIZE / m_sprite->getContentSize().height);
            if (m_dragging) {
                auto pos = m_sprite->getPosition();
                Mod::get()->setSavedValue("mobile-button-x", pos.x);
                Mod::get()->setSavedValue("mobile-button-y", pos.y);
            } else {
                tanzanite::toggleMenu();
            }
            m_dragging = false;
        }

        void ccTouchCancelled(CCTouch* touch, CCEvent* event) override {
            m_sprite->setScale(SIZE / m_sprite->getContentSize().height);
            m_dragging = false;
        }
    };

    bool s_added = false;
}

// Add the button once, as soon as the main menu exists
class $modify(MobileMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (!s_added) {
            s_added = true;
            if (auto btn = MobileButton::create()) OverlayManager::get()->addChild(btn);
        }
        return true;
    }
};

#endif
