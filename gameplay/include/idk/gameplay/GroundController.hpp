#pragma once

#include "idk/engine/CharacterBody.hpp"
#include "idk/engine/CharacterController.hpp"
#include "libidk/Types.hpp"

namespace idk::gameplay
{
    // Shared horizontal movement and jump handling. Concrete controllers choose
    // how input or AI produces a direction. The borrowed body must outlive this.
    class GroundController: public engine::CharacterController, public idk::Immobile
    {
    public:
        static constexpr float WalkSpeed = 6.0f;
        static constexpr float RunSpeed = 10.0f;

        bool jump() override;
        void step(float dt) override;
        void clearInput() override;

    protected:
        explicit GroundController(engine::CharacterBody &body) : mBody(body) {}

        void resetBody(glm::vec3 feet);
        // World-space XZ direction. Magnitudes below one preserve analog input.
        void setMovement(glm::vec3 direction, bool sprint);

        const engine::CharacterBody &body() const { return mBody; }

    private:
        engine::CharacterBody &mBody;
        glm::vec3 mMovement{0};
        bool mJump = false;
    };
}
