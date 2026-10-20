#pragma once

#include "GroundController.hpp"

namespace idk::gameplay
{
    // First-person movement policy, independent of levels and physics backends.
    // The body is borrowed and must outlive the controller.
    class PlayerController final: public GroundController
    {
    public:
        explicit PlayerController(engine::CharacterBody &body) : GroundController(body) {}

        void reset(glm::vec3 feet, float yaw = 0, float pitch = 0);
        void move(float right, float forward, bool sprint) override;
        void look(float yaw, float pitch) override;

        float yaw() const { return mYaw; }

        float pitch() const { return mPitch; }

    private:
        float mYaw = 0, mPitch = 0;
    };
}
