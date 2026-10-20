#pragma once

namespace idk::engine
{
    // Receives intent from input, AI or commands without depending on a device
    // or physics backend. Implementations retain intent until step() consumes it.
    class CharacterController
    {
    public:
        virtual ~CharacterController() = default;

        // Movement replaces the previous request; look applies a delta in degrees.
        virtual void move(float right, float forward, bool sprint) = 0;
        virtual void look(float yaw, float pitch) = 0;
        virtual void step(float dt) = 0;

        // Cancel pending actions and held movement without changing orientation.
        virtual void clearInput() = 0;

        // Optional actions return whether the request was accepted, not whether
        // it succeeded physically. Unsupported actions return false.
        virtual bool jump() { return false; }

        virtual bool crouch(bool) { return false; }

        virtual bool attack() { return false; }
    };
}
