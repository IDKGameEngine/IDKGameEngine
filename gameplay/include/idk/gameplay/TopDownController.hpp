#pragma once

#include "GroundController.hpp"

namespace idk::gameplay
{
    // Movement follows fixed world axes (+X right, -Z forward), independently
    // of aiming. Look changes aim yaw; pitch is unused.
    class TopDownController final: public GroundController
    {
    public:
        explicit TopDownController(engine::CharacterBody &body) : GroundController(body) {}

        void reset(glm::vec3 feet, float yaw = 0);
        void move(float right, float forward, bool sprint) override;
        void look(float yaw, float pitch) override;

        float yaw() const { return mYaw; }

    private:
        float mYaw = 0;
    };
}
