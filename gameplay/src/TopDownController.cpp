#include "idk/gameplay/TopDownController.hpp"

namespace idk::gameplay
{
    void TopDownController::reset(glm::vec3 feet, float yaw)
    {
        resetBody(feet);
        mYaw = yaw;
    }

    void TopDownController::move(float right, float forward, bool sprint)
    {
        setMovement({right, 0, -forward}, sprint);
    }

    void TopDownController::look(float yaw, float)
    {
        mYaw += yaw;
    }
}
