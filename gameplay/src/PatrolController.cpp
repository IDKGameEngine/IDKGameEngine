#include "idk/gameplay/PatrolController.hpp"
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <algorithm>
#include <cmath>

namespace idk::gameplay
{
    bool PatrolController::setWaypoints(std::span<const glm::vec3> waypoints, bool loop)
    {
        if (waypoints.empty() || waypoints.size() > MaxWaypoints)
            return false;
        idk::FixedList<glm::vec3, MaxWaypoints> path;
        for (auto point : waypoints)
        {
            if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z))
                return false;
            path.push(point);
        }
        mWaypoints = path;
        mLoop = loop;
        start();
        return true;
    }

    void PatrolController::start()
    {
        GroundController::clearInput();
        mWaypoint = 0;
        mActive = !mWaypoints.empty();
    }

    void PatrolController::reset(glm::vec3 feet)
    {
        resetBody(feet);
        mYaw = 0;
        start();
    }

    void PatrolController::clearInput()
    {
        GroundController::clearInput();
        mActive = false;
    }

    void PatrolController::steer(float dt)
    {
        setMovement({}, false);
        if (!mActive)
            return;
        auto position = body().position();
        // Bound the scan even if a looping route contains only coincident points.
        for (size_t visited = 0; visited < mWaypoints.size(); ++visited)
        {
            glm::vec3 offset = mWaypoints[mWaypoint] - position;
            offset.y = 0;
            float distance = glm::length(offset);
            if (distance > ArrivalDistance)
            {
                // Ask for no more than the remaining distance in this tick.
                float amount = std::min(1.0f, distance / (WalkSpeed * dt));
                setMovement((offset / distance) * amount, false);
                mYaw = glm::degrees(std::atan2(-offset.x, -offset.z));
                return;
            }
            if (mWaypoint + 1 < mWaypoints.size())
                ++mWaypoint;
            else if (mLoop)
                mWaypoint = 0;
            else
            {
                mActive = false;
                return;
            }
        }
    }

    void PatrolController::step(float dt)
    {
        if (!std::isfinite(dt) || dt <= 0)
            return;
        steer(dt);
        GroundController::step(dt);
    }
}
