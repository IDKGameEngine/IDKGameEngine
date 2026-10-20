#include "idk/gameplay/PlayerController.hpp"
#include "idk/engine/CharacterCommand.hpp"
#include "idk/engine/CharacterInputHandler.hpp"
#include <glm/geometric.hpp>
#include <cstdio>
#include <limits>
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
    // Deliberately has no level, renderer or Jolt dependencies. Movement is
    // integrated directly so the same controller can be exercised in isolation.
    class TestBody final: public idk::engine::CharacterBody
    {
    public:
        glm::vec3 feet{0};
        unsigned steps = 0, jumps = 0;

        void reset(glm::vec3 position) override { feet = position; }

        glm::vec3 position() const override { return feet; }

        void step(glm::vec3 velocity, bool jump, float dt) override
        {
            feet += velocity * dt;
            ++steps;
            if (jump)
                ++jumps;
        }
    };

    bool checks()
    {
        using namespace idk::engine;
        static_assert(std::is_abstract_v<CharacterBody>);
        TestBody body;
        idk::gameplay::PlayerController controller(body);
        JumpCommand jump;
        CharacterInputHandler input(controller);
        CHECK(input.bind(0, &jump));
        controller.reset({1, 2, 3});
        CHECK(body.position() == glm::vec3(1, 2, 3));

        // Look, movement and a short jump press reach the alternate body through
        // the same command/input path used by the playtest.
        CharacterInput frame;
        frame.moveY = 1;
        frame.lookX = 90;
        frame.buttons[0] = true;
        input.update(frame);
        frame.lookX = 0;
        frame.buttons[0] = false;
        input.update(frame);
        CHECK(body.steps == 0 && body.jumps == 0);
        controller.step(0);
        controller.step(-1);
        controller.step(std::numeric_limits<float>::infinity());
        controller.step(std::numeric_limits<float>::quiet_NaN());
        CHECK(body.steps == 0 && body.jumps == 0);
        controller.step(.25f);
        CHECK(glm::distance(body.position(), glm::vec3(-.5f, 2, 3)) < .001f);
        CHECK(body.steps == 1 && body.jumps == 1);
        controller.step(.25f);
        CHECK(glm::distance(body.position(), glm::vec3(-2, 2, 3)) < .001f);
        CHECK(body.steps == 2 && body.jumps == 1);

        // Input cancellation reaches the body on the next tick, without
        // teleporting it or replaying a pending jump.
        frame.buttons[0] = true;
        input.update(frame);
        input.reset();
        auto previous = body.position();
        controller.step(.25f);
        CHECK(body.position() == previous && body.jumps == 1);

        // A respawn resets both the body and controller intent. Button history
        // remains in the input handler, so a held key does not cause a new jump.
        input.update(frame);
        controller.reset({4, 5, 6}, 180, 100);
        CHECK(body.position() == glm::vec3(4, 5, 6));
        CHECK(controller.yaw() == 180 && controller.pitch() == 85);
        controller.step(.25f);
        CHECK(body.position() == glm::vec3(4, 5, 6) && body.jumps == 1);
        frame.moveY = 0;
        input.update(frame);
        controller.step(.25f);
        CHECK(body.position() == glm::vec3(4, 5, 6) && body.jumps == 1);
        return true;
    }
}

int main()
{
    if (!checks())
        return 1;
    std::puts("Gameplay controller checks passed with an independent character body");
    return 0;
}
