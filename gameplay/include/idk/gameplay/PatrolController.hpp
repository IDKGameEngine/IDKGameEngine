#pragma once

#include "GroundController.hpp"
#include "libidk/dsa/FixedList.hpp"
#include <span>

namespace idk::gameplay
{
    // Walks directly between world-space waypoints on XZ. This is simple
    // steering, without pathfinding or obstacle avoidance. Actions still use
    // the command interface; manual movement/look input is ignored.
    class PatrolController final: public GroundController
    {
    public:
        static constexpr size_t MaxWaypoints = 32;
        static constexpr float ArrivalDistance = .1f;

        explicit PatrolController(engine::CharacterBody &body) : GroundController(body) {}

        // Copies and starts the route. Invalid input leaves the old route intact.
        bool setWaypoints(std::span<const glm::vec3> waypoints, bool loop = true);
        // Restart the stored route at its first waypoint.
        void start();
        // Reset the body, orientation and route progress.
        void reset(glm::vec3 feet);

        void move(float, float, bool) override {}

        void look(float, float) override {}

        void step(float dt) override;
        // Stop the route and cancel actions. start() restarts it explicitly.
        void clearInput() override;

        bool active() const { return mActive; }

        float yaw() const { return mYaw; }

    private:
        idk::FixedList<glm::vec3, MaxWaypoints> mWaypoints;
        size_t mWaypoint = 0;
        float mYaw = 0;
        bool mLoop = true, mActive = false;

        void steer(float dt);
    };
}
