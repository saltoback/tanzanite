// Tanzanite — Show Trajectory
#include "Trajectory.hpp"
#include "PlayerSnapshot.hpp"
#include "State.hpp"
#include "Ui.hpp"

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;
using namespace tanzanite;

namespace {
    constexpr int LENGTH = 180;            // simulated physics steps (0.75 s)

    bool s_sim = false;                    // a simulation is running
    bool s_simDead = false;                // the fake player died during it
    float s_delta = 0.f;                   // step delta of the real player
    PlayerObject* s_fake = nullptr;        // invisible copy of the player
    CCDrawNode* s_node = nullptr;          // where the paths are drawn

    void drawCross(CCPoint p, ccColor4F color) {
        constexpr float S = 5.f;
        s_node->drawSegment({p.x - S, p.y - S}, {p.x + S, p.y + S}, 0.8f, color);
        s_node->drawSegment({p.x - S, p.y + S}, {p.x + S, p.y - S}, 0.8f, color);
    }

    void simulatePath(PlayLayer* pl, PlayerObject* real, bool hold, ccColor4F color) {
        s_sim = true;
        s_simDead = false;

        snap::copyForSimulation(real, s_fake);
        s_fake->m_isDead = false;
        s_fake->m_playEffects = false;
        s_fake->setVisible(false);

        for (int i = 0; i < LENGTH; i++) {
            CCPoint prev = s_fake->getPosition();

            s_fake->m_collisionLogTop->removeAllObjects();
            s_fake->m_collisionLogBottom->removeAllObjects();
            s_fake->m_collisionLogLeft->removeAllObjects();
            s_fake->m_collisionLogRight->removeAllObjects();

            pl->checkCollisions(s_fake, s_delta, false);
            if (s_simDead) {
                drawCross(prev, color);
                break;
            }

            if (hold) s_fake->pushButton(PlayerButton::Jump);
            else s_fake->releaseButton(PlayerButton::Jump);

            s_fake->update(s_delta);
            s_fake->updateRotation(s_delta);

            s_node->drawSegment(prev, s_fake->getPosition(), 0.6f, color);
        }

        s_sim = false;
    }
}

bool traj::simulating() { return s_sim; }
void traj::onSimDeath() { s_simDead = true; }

void traj::onLevelInit(PlayLayer* pl) {
    traj::onQuit();

    s_fake = PlayerObject::create(1, 1, pl, pl->m_objectLayer, true);
    if (s_fake) {
        s_fake->retain();
        s_fake->setVisible(false);
        s_fake->setPosition({0.f, 105.f});
        pl->m_objectLayer->addChild(s_fake);
    }

    s_node = ui::makeDraw();
    s_node->retain();
    s_node->setID("trajectory"_spr);
    pl->m_objectLayer->addChild(s_node, 1000);
}

void traj::onQuit() {
    if (s_fake) {
        s_fake->removeFromParent();
        s_fake->release();
        s_fake = nullptr;
    }
    if (s_node) {
        s_node->removeFromParent();
        s_node->release();
        s_node = nullptr;
    }
    s_delta = 0.f;
    s_sim = false;
}

void traj::update(PlayLayer* pl) {
    if (!s_node) return;
    s_node->clear();
    if (!s.showTrajectory || !s_fake || s_delta <= 0.f) return;
    if (pl->m_isPaused || pl->m_player1->m_isDead || pl->m_levelEndAnimationStarted) return;

    simulatePath(pl, pl->m_player1, true, {0.3f, 1.f, 0.4f, 0.95f});
    simulatePath(pl, pl->m_player1, false, {1.f, 0.35f, 0.35f, 0.95f});

    if (pl->m_gameState.m_isDualMode) {
        simulatePath(pl, pl->m_player2, true, {0.3f, 0.9f, 1.f, 0.95f});
        simulatePath(pl, pl->m_player2, false, {1.f, 0.6f, 0.2f, 0.95f});
    }
}

// ---------------------------------------------------------------------------
// While simulating, the fake player must not change anything in the level:
// no portals, orbs, pads, triggers, collectibles or end animation.
// ---------------------------------------------------------------------------
class $modify(TrajPlayerObject, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);
        if (s_sim) return;
        auto pl = PlayLayer::get();
        if (pl && this == pl->m_player1) s_delta = dt;
    }
};

class $modify(TrajGameLayer, GJBaseGameLayer) {
    bool canBeActivatedByPlayer(PlayerObject* player, EffectGameObject* object) {
        if (s_sim) return false;
        return GJBaseGameLayer::canBeActivatedByPlayer(player, object);
    }
    void gameEventTriggered(GJGameEvent event, int material, int playerID) {
        if (s_sim) return;
        GJBaseGameLayer::gameEventTriggered(event, material, playerID);
    }
    void checkpointActivated(CheckpointGameObject* object) {
        if (s_sim) return;
        GJBaseGameLayer::checkpointActivated(object);
    }
    void bumpPlayer(PlayerObject* player, EffectGameObject* object) {
        if (s_sim) return;
        GJBaseGameLayer::bumpPlayer(player, object);
    }
    void destroyObject(GameObject* object) {
        if (s_sim) return;
        GJBaseGameLayer::destroyObject(object);
    }
    void pickupItem(EffectGameObject* object) {
        if (s_sim) return;
        GJBaseGameLayer::pickupItem(object);
    }
    void playerTouchedRing(PlayerObject* player, RingObject* object) {
        if (s_sim) return;
        GJBaseGameLayer::playerTouchedRing(player, object);
    }
    void playerTouchedTrigger(PlayerObject* player, EffectGameObject* object) {
        if (s_sim) return;
        GJBaseGameLayer::playerTouchedTrigger(player, object);
    }
    void playerWillSwitchMode(PlayerObject* player, GameObject* object) {
        if (s_sim) return;
        GJBaseGameLayer::playerWillSwitchMode(player, object);
    }
    void flipGravity(PlayerObject* player, bool flip, bool noEffects) {
        if (s_sim) return;
        GJBaseGameLayer::flipGravity(player, flip, noEffects);
    }
    void switchToRobotMode(PlayerObject* player, GameObject* object, bool noPortal) {
        if (s_sim) return;
        GJBaseGameLayer::switchToRobotMode(player, object, noPortal);
    }
    void switchToRollMode(PlayerObject* player, GameObject* object, bool noPortal) {
        if (s_sim) return;
        GJBaseGameLayer::switchToRollMode(player, object, noPortal);
    }
    void switchToSpiderMode(PlayerObject* player, GameObject* object, bool noPortal) {
        if (s_sim) return;
        GJBaseGameLayer::switchToSpiderMode(player, object, noPortal);
    }
    void teleportPlayer(TeleportPortalObject* object, PlayerObject* player) {
        if (s_sim) return;
        GJBaseGameLayer::teleportPlayer(object, player);
    }
    void toggleDualMode(GameObject* object, bool dual, PlayerObject* player, bool noEffects) {
        if (s_sim) return;
        GJBaseGameLayer::toggleDualMode(object, dual, player, noEffects);
    }
};

class $modify(TrajPlayLayer, PlayLayer) {
    void playEndAnimationToPos(CCPoint position) {
        if (s_sim) return;
        PlayLayer::playEndAnimationToPos(position);
    }
};
