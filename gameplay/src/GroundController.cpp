#include "idk/gameplay/GroundController.hpp"
#include <glm/geometric.hpp>
#include <cmath>

namespace idk::gameplay
{
    void GroundController::resetBody(glm::vec3 feet)
    {
        mBody.reset(feet);
        clearInput();
    }

    void GroundController::setMovement(glm::vec3 direction, bool sprint)
    {
        direction.y = 0;
        float length = glm::length(direction);
        if (length > 1)
            direction /= length;
        mMovement = direction * (sprint ? RunSpeed : WalkSpeed);
    }

    bool GroundController::jump()
    {
        mJump = true;
        return true;
    }

    void GroundController::step(float dt)
    {
        if (!std::isfinite(dt) || dt <= 0)
            return;
        mBody.step(mMovement, mJump, dt);
        mJump = false;
    }

    void GroundController::clearInput()
    {
        mMovement = {};
        mJump = false;
    }
}
