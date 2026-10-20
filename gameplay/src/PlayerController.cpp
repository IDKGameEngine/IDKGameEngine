#include "idk/gameplay/PlayerController.hpp"
#include <glm/trigonometric.hpp>
#include <algorithm>
#include <cmath>

namespace idk::gameplay
{
    void PlayerController::reset(glm::vec3 feet, float yaw, float pitch)
    {
        resetBody(feet);
        mYaw = yaw;
        mPitch = std::clamp(pitch, -85.0f, 85.0f);
    }

    void PlayerController::move(float right, float forward, bool sprint)
    {
        float yaw = glm::radians(mYaw);
        glm::vec3 forwardAxis(-std::sin(yaw), 0, -std::cos(yaw));
        glm::vec3 rightAxis(-forwardAxis.z, 0, forwardAxis.x);
        setMovement(rightAxis * right + forwardAxis * forward, sprint);
    }

    void PlayerController::look(float yaw, float pitch)
    {
        mYaw += yaw;
        mPitch = std::clamp(mPitch + pitch, -85.0f, 85.0f);
    }
}
