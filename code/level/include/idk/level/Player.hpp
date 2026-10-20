#pragma once

#include "Level.hpp"
#include "PhysicsWorld.hpp"
#include "idk/engine/CharacterBody.hpp"
#include "libidk/Types.hpp"

namespace JPH
{
    class CharacterVirtual;
}

namespace idk::level
{
    // A 1.8m capsule with a 0.3m radius. Position denotes the player's feet.
    // Jolt owns collision storage; update scratch uses a bounded 8 MiB budget.
    class Player final: public engine::CharacterBody, public idk::Immobile
    {
    public:
        ~Player() override;
        bool initialize(PhysicsWorld &, Error &);
        // Minimum substep frequency, 30..1000 Hz. Invalid settings are ignored.
        bool setPhysicsRate(double rateHz);
        void shutdown();
        void reset(glm::vec3 feet) override;
        // Desired horizontal velocity; airborne movement preserves momentum.
        void step(glm::vec3 wishVelocity, bool jump, float dt) override;
        glm::vec3 position() const override;
        // Render between fixed positions; reset() snaps both to the new spawn.
        glm::vec3 interpolatedPosition(float alpha) const;
        bool grounded() const;

    private:
        PhysicsWorld *mWorld = nullptr;
        JPH::CharacterVirtual *mCharacter = nullptr; // One owned Jolt reference.
        glm::vec3 mPreviousPosition{0};
        double mPhysicsRateHz = 120;
    };
} // namespace idk::level
