// Tanzanite — shared UI pieces (rounded panels, pills, switches)
#pragma once

#include "State.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace tanzanite::ui {
    using namespace geode::prelude;

    inline ccColor4F rgba(float r, float g, float b, float a) { return {r, g, b, a}; }

    // CCDrawNode blends as premultiplied alpha by default, which turns
    // translucent colors opaque. Use regular alpha blending instead.
    inline CCDrawNode* makeDraw() {
        auto dn = CCDrawNode::create();
        dn->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
        return dn;
    }

    // Rounded rectangle
    inline void drawRoundedRect(CCDrawNode* dn, CCRect r, float rad, ccColor4F fill, float bw, ccColor4F border) {
        constexpr float PI = 3.14159265f;
        constexpr int SEG = 6;
        std::vector<CCPoint> pts;
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

    inline CCLabelBMFont* makeLabel(char const* text, char const* font, float scale, ccColor3B color = {255, 255, 255}) {
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

    // Rounded pill button (tabs, mode buttons, actions)
    class PillNode : public CCNode {
        CCDrawNode* m_draw = nullptr;
        CCLabelBMFont* m_label = nullptr;
        float m_w = 86.f, m_h = 24.f;
        float m_scale = 0.4f;
        bool m_selected = false;
    public:
        static PillNode* create(char const* text, float w = 86.f, float h = 24.f, float scale = 0.4f) {
            auto ret = new PillNode();
            if (ret->init(text, w, h, scale)) { ret->autorelease(); return ret; }
            delete ret;
            return nullptr;
        }
        bool init(char const* text, float w, float h, float scale) {
            if (!CCNode::init()) return false;
            m_w = w;
            m_h = h;
            m_scale = scale;
            this->setContentSize({w, h});
            this->setAnchorPoint({0.5f, 0.5f});
            m_draw = makeDraw();
            this->addChild(m_draw);
            m_label = makeLabel(text, "bigFont.fnt", scale);
            m_label->setPosition({w / 2.f, h / 2.f});
            this->addChild(m_label);
            this->setText(text);
            this->redraw();
            return true;
        }
        void setText(char const* text) {
            m_label->setString(text);
            m_label->limitLabelWidth(m_w - 8.f, m_scale, 0.1f);
        }
        void setSelected(bool sel) {
            if (sel == m_selected) return;
            m_selected = sel;
            this->redraw();
        }
        bool isSelected() const { return m_selected; }
        void redraw() {
            m_draw->clear();
            float rad = std::min(6.f, m_h / 2.f);
            if (m_selected) {
                drawRoundedRect(m_draw, {0.f, 0.f, m_w, m_h}, rad,
                    rgba(0.59f, 0.43f, 1.f, 0.85f), 0.8f, rgba(0.85f, 0.78f, 1.f, 1.f));
                m_label->setColor({255, 255, 255});
            } else {
                drawRoundedRect(m_draw, {0.f, 0.f, m_w, m_h}, rad,
                    rgba(0.08f, 0.05f, 0.16f, 0.55f), 0.6f, rgba(0.59f, 0.43f, 1.f, 0.45f));
                m_label->setColor({200, 190, 235});
            }
        }
    };
}
