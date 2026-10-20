#include "idk/gameplay/SideScrollerController.hpp"

namespace idk::gameplay
{
    void SideScrollerController::reset(glm::vec3 feet)
    {
        resetBody(feet);
        mFacing = 1;
    }

    void SideScrollerController::move(float right, float, bool sprint)
    {
        setMovement({right, 0, 0}, sprint);
        if (right != 0)
            mFacing = right < 0 ? -1 : 1;
    }
}
