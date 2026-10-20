#pragma once

#include <glm/vec3.hpp>

namespace idk::engine
{
    // Physics-facing operations used by gameplay controllers. Implementations
    // own collision state and decide how to satisfy movement and jump requests.
    class CharacterBody
    {
    public:
        virtual ~CharacterBody() = default;

        // Teleport the feet to a world position and clear physical momentum.
        virtual void reset(glm::vec3 feet) = 0;
        // Current world-space feet position, for autonomous steering.
        virtual glm::vec3 position() const = 0;
        // Desired world-space horizontal velocity and a jump request for this
        // tick only. dt is a positive, finite duration in seconds.
        virtual void step(glm::vec3 wishVelocity, bool jump, float dt) = 0;
    };
}
