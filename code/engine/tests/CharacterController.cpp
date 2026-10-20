#include "idk/engine/CharacterCommand.hpp"
#include "idk/engine/CharacterInputHandler.hpp"
#include <cstdio>
#include <type_traits>

#define CHECK(value)                                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(value))                                                                                                  \
        {                                                                                                              \
            std::fprintf(stderr, "Check failed: %s (%d)\n", #value, __LINE__);                                         \
            return false;                                                                                              \
        }                                                                                                              \
    } while (false)

namespace
{
    using namespace idk::engine;

    // A receiver with no physics or input-device dependencies.
    class TestController final: public CharacterController
    {
    public:
        float right = 0, forward = 0, yaw = 0, pitch = 0, movementYaw = 0;
        bool sprint = false, crouching = false;
        unsigned jumps = 0, attacks = 0, crouchChanges = 0, clears = 0;

        void move(float x, float y, bool running) override
        {
            right = x;
            forward = y;
            sprint = running;
            movementYaw = yaw;
        }

        void look(float x, float y) override
        {
            yaw += x;
            pitch += y;
        }

        bool jump() override
        {
            ++jumps;
            return true;
        }

        bool crouch(bool active) override
        {
            crouching = active;
            ++crouchChanges;
            return true;
        }

        bool attack() override
        {
            ++attacks;
            return true;
        }

        void step(float) override {}

        void clearInput() override
        {
            right = forward = 0;
            sprint = crouching = false;
            ++clears;
        }
    };

    // New command types need no changes to the input handler.
    class JumpAttackCommand final: public CharacterCommand
    {
    public:
        bool execute(CharacterController &controller) const override
        {
            bool jumped = controller.jump();
            bool attacked = controller.attack();
            return jumped && attacked;
        }
    };

    bool checks()
    {
        static_assert(std::is_abstract_v<CharacterController>);
        static_assert(std::is_abstract_v<CharacterCommand>);
        TestController controller;
        JumpCommand jump;
        CrouchCommand crouch(true), stand(false);
        AttackCommand attack;
        JumpAttackCommand combo;
        CharacterInputHandler handler(controller);
        CHECK(handler.bind(0, &jump));
        CHECK(handler.bind(1, &crouch, &stand));
        CHECK(handler.bind(2, &attack));
        CHECK(!handler.bind(CharacterInput::MaxButtons, &jump));
        CHECK(!handler.bind(size_t(-1), &jump));

        CharacterInput input;
        input.moveX = .5f;
        input.moveY = -1;
        input.lookX = 30;
        input.lookY = -10;
        input.sprint = true;
        input.buttons[0] = input.buttons[1] = input.buttons[2] = true;
        handler.update(input);
        CHECK(controller.right == .5f && controller.forward == -1 && controller.sprint);
        CHECK(controller.yaw == 30 && controller.pitch == -10 && controller.movementYaw == 30);
        CHECK(controller.jumps == 1 && controller.attacks == 1);
        CHECK(controller.crouching && controller.crouchChanges == 1);

        // Held buttons do not retrigger; rebinding one cannot lose its release.
        CHECK(!handler.bind(1, &attack));
        input.lookX = input.lookY = 0;
        for (unsigned frame = 0; frame < 10; ++frame)
            handler.update(input);
        CHECK(controller.jumps == 1 && controller.attacks == 1 && controller.crouchChanges == 1);
        CHECK(controller.yaw == 30 && controller.pitch == -10);
        input.buttons[1] = false;
        handler.update(input);
        CHECK(!controller.crouching && controller.crouchChanges == 2);
        input.buttons[1] = true;
        handler.update(input);
        CHECK(controller.crouching && controller.crouchChanges == 3);

        // Capture loss releases held actions once and keeps the camera orientation.
        handler.reset();
        CHECK(!controller.crouching && controller.crouchChanges == 4);
        CHECK(controller.right == 0 && controller.forward == 0 && !controller.sprint);
        CHECK(controller.yaw == 30 && controller.pitch == -10 && controller.clears == 1);
        handler.reset();
        CHECK(controller.crouchChanges == 4 && controller.clears == 2);

        // A button can be remapped to an unrelated command or unbound entirely.
        CHECK(handler.bind(0, &attack));
        input = {};
        input.buttons[0] = true;
        handler.update(input);
        CHECK(controller.jumps == 1 && controller.attacks == 2);
        input.buttons[0] = false;
        handler.update(input);
        CHECK(handler.bind(0, nullptr));
        input.buttons[0] = true;
        handler.update(input);
        CHECK(controller.jumps == 1 && controller.attacks == 2);

        constexpr size_t customButton = CharacterInput::MaxButtons - 1;
        CHECK(handler.bind(customButton, &combo));
        input.buttons[customButton] = true;
        handler.update(input);
        CHECK(controller.jumps == 2 && controller.attacks == 3);

        // Commands have no stored receiver: reuse them for another character or AI.
        TestController other;
        CharacterInputHandler otherHandler(other);
        CHECK(otherHandler.bind(0, &jump));
        otherHandler.update(input);
        CHECK(other.jumps == 1 && controller.jumps == 2);
        CHECK(attack.execute(other));
        CHECK(other.attacks == 1 && controller.attacks == 3);
        return true;
    }
}

int main()
{
    if (!checks())
        return 1;
    std::puts("Character command bindings, press/release, remapping and reset checks passed");
    return 0;
}
