// Tanzanite — full copy of a player's physics state
// Used for practice checkpoints (Bot.cpp) and for the trajectory (Trajectory.cpp).
#pragma once

#include <Geode/Geode.hpp>

namespace tanzanite::snap {
    using namespace geode::prelude;

#define TZ_PLAYER_FIELDS(X) \
    X(position) X(lastGroundedPos) X(yVelocity) X(platformerXVelocity) X(fallSpeed) \
    X(gravity) X(gravityMod) X(isOnGround) X(isOnGround2) X(isOnGround3) X(isOnGround4) \
    X(isUpsideDown) X(isDashing) X(dashX) X(dashY) X(dashAngle) X(dashStartTime) \
    X(vehicleSize) X(playerSpeed) X(isSliding) X(isSlidingRight) X(isRotating) \
    X(rotationSpeed) X(lastJumpTime) X(lastFlipTime) X(lastLandTime) X(isAccelerating) \
    X(accelerationOrSpeed) X(jumpBuffered) X(stateJumpBuffered) X(yVelocityBeforeSlope) \
    X(currentSlopeYVelocity) X(groundYVelocity) X(lastSpiderFlipTime) X(isGoingLeft) \
    X(hasEverJumped)

// Game mode flags: only copied onto the trajectory's fake player
#define TZ_MODE_FIELDS(X) \
    X(isShip) X(isBall) X(isBird) X(isSwing) X(isDart) X(isRobot) X(isSpider) \
    X(isSideways) X(isPlatformer) X(holdingLeft) X(holdingRight)

    struct FullState {
#define TZ_DECLARE(f) decltype(PlayerObject::m_##f) f{};
        TZ_PLAYER_FIELDS(TZ_DECLARE)
#undef TZ_DECLARE
        CCPoint nodePos;
        float rotation = 0.f;
        float scale = 1.f;
    };

    inline FullState snapshot(PlayerObject* p) {
        FullState st;
#define TZ_SAVE(f) st.f = p->m_##f;
        TZ_PLAYER_FIELDS(TZ_SAVE)
#undef TZ_SAVE
        st.nodePos = p->getPosition();
        st.rotation = p->getRotation();
        st.scale = p->getScale();
        return st;
    }

    inline void restore(PlayerObject* p, FullState const& st) {
        p->setPosition(st.nodePos);
        p->setRotation(st.rotation);
#define TZ_LOAD(f) p->m_##f = st.f;
        TZ_PLAYER_FIELDS(TZ_LOAD)
#undef TZ_LOAD
    }

    // Make `to` an exact physics copy of `from` (game mode included)
    inline void copyForSimulation(PlayerObject* from, PlayerObject* to) {
#define TZ_COPY_MODE(f) to->m_##f = from->m_##f;
        TZ_MODE_FIELDS(TZ_COPY_MODE)
#undef TZ_COPY_MODE
        auto st = snapshot(from);
        restore(to, st);
        to->setScale(st.scale);
    }
}
