#pragma once

#include "GroundController.hpp"

namespace idk::gameplay
{
    // Horizontal input controls world X; forward and look input are unused.
    // The body supplies gravity/jumping and any physical constraint to a plane.
    class SideScrollerController final: public GroundController
    {
    public:
        explicit SideScrollerController(engine::CharacterBody &body) : GroundController(body) {}

        void reset(glm::vec3 feet);
        void move(float right, float forward, bool sprint) override;

        void look(float, float) override {}

        // -1 faces left, +1 faces right. Idle input preserves the last facing.
        int facing() const { return mFacing; }

    private:
        int mFacing = 1;
    };
}
