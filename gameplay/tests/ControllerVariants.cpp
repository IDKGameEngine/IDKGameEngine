#include "idk/gameplay/PlayerController.hpp"
#include "idk/gameplay/TopDownController.hpp"
#include "idk/gameplay/SideScrollerController.hpp"
#include "idk/gameplay/PatrolController.hpp"
#include "idk/engine/CharacterCommand.hpp"
#include "idk/engine/CharacterInputHandler.hpp"
#include <glm/geometric.hpp>
#include <cmath>
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
    using namespace idk::engine;
    using namespace idk::gameplay;

    class TestBody final: public CharacterBody
    {
    public:
        glm::vec3 feet{0}, velocity{0};
        unsigned steps = 0, jumps = 0;

        void reset(glm::vec3 position) override
        {
            feet = position;
            velocity = {};
        }

        glm::vec3 position() const override { return feet; }

        void step(glm::vec3 movement, bool jump, float dt) override
        {
            velocity = movement;
            feet += movement * dt;
            ++steps;
            if (jump)
                ++jumps;
        }
    };

    bool close(glm::vec3 actual, glm::vec3 expected)
    {
        return glm::distance(actual, expected) < .001f;
    }

    bool commandChecks(CharacterController &controller, TestBody &body)
    {
        JumpCommand jump;
        CharacterInputHandler input(controller);
        CHECK(input.bind(0, &jump));
        CharacterInput frame;
        frame.buttons[0] = true;
        input.update(frame);
        controller.step(0);
        controller.step(-1);
        controller.step(std::numeric_limits<float>::quiet_NaN());
        controller.step(std::numeric_limits<float>::infinity());
        CHECK(body.steps == 0 && body.jumps == 0);
        controller.step(.1f);
        CHECK(body.jumps == 1);
        input.update(frame);
        controller.step(.1f);
        CHECK(body.jumps == 1);
        frame.buttons[0] = false;
        input.update(frame);
        frame.buttons[0] = true;
        input.update(frame);
        input.reset();
        controller.step(.1f);
        CHECK(body.jumps == 1 && body.velocity == glm::vec3(0));
        return true;
    }

    bool movementChecks()
    {
        static_assert(std::is_abstract_v<GroundController>);
        TestBody firstPersonBody, topDownBody, sideBody, patrolBody;
        PlayerController firstPerson(firstPersonBody);
        TopDownController topDown(topDownBody);
        SideScrollerController side(sideBody);
        PatrolController patrol(patrolBody);
        CHECK(commandChecks(firstPerson, firstPersonBody));
        CHECK(commandChecks(topDown, topDownBody));
        CHECK(commandChecks(side, sideBody));
        CHECK(commandChecks(patrol, patrolBody));

        // The same input frame produces local movement in first person and
        // world movement in top-down mode, while both update their aim yaw.
        CharacterInput frame;
        frame.lookX = 90;
        frame.lookY = 70;
        frame.moveY = 1;
        CharacterInputHandler firstPersonInput(firstPerson), topDownInput(topDown);
        firstPersonInput.update(frame);
        topDownInput.update(frame);
        firstPerson.step(.1f);
        topDown.step(.1f);
        CHECK(close(firstPersonBody.velocity, {-6, 0, 0}));
        CHECK(close(topDownBody.velocity, {0, 0, -6}));
        CHECK(topDown.yaw() == 90);

        topDown.move(.5f, 0, false);
        topDown.step(.1f);
        CHECK(close(topDownBody.velocity, {3, 0, 0}));
        topDown.move(1, 1, true);
        topDown.step(.1f);
        CHECK(std::abs(glm::length(topDownBody.velocity) - 10) < .001f);
        CHECK(std::abs(topDownBody.velocity.x + topDownBody.velocity.z) < .001f);
        CHECK(topDown.jump());
        topDown.reset({2, 3, 4}, 45);
        topDown.step(.1f);
        CHECK(topDownBody.position() == glm::vec3(2, 3, 4));
        CHECK(topDownBody.jumps == 1 && topDown.yaw() == 45);

        side.look(90, 70);
        side.move(-.5f, 1, true);
        side.step(.1f);
        CHECK(close(sideBody.velocity, {-5, 0, 0}) && side.facing() == -1);
        side.move(0, 1, false);
        side.step(.1f);
        CHECK(sideBody.velocity == glm::vec3(0) && side.facing() == -1);
        side.move(2, -1, false);
        side.step(.1f);
        CHECK(close(sideBody.velocity, {6, 0, 0}) && side.facing() == 1);
        CHECK(side.jump());
        side.reset({3, 2, 1});
        side.step(.1f);
        CHECK(sideBody.position() == glm::vec3(3, 2, 1));
        CHECK(sideBody.jumps == 1 && side.facing() == 1);
        return true;
    }

    bool patrolChecks()
    {
        TestBody body;
        PatrolController patrol(body);
        patrol.start();
        CHECK(!patrol.active());
        // The first point is already reached horizontally. Height does not
        // affect route progress, and subsequent edits to the array have no effect.
        glm::vec3 route[] = {{0, 50, 0}, {1, 0, 0}, {1, 0, -1}};
        CHECK(patrol.setWaypoints(route, false));
        route[1].x = 100;
        patrol.step(0);
        CHECK(body.steps == 0 && patrol.active());
        patrol.move(-1, 1, true);
        patrol.look(180, 90);
        patrol.step(.25f);
        CHECK(close(body.position(), {1, 0, 0}));
        CHECK(patrol.yaw() == -90);
        patrol.step(.25f);
        CHECK(close(body.position(), {1, 0, -1}));
        patrol.step(.25f);
        CHECK(!patrol.active() && body.velocity == glm::vec3(0));

        const glm::vec3 loop[] = {{0, 0, 0}, {1, 0, 0}};
        patrol.reset({0, 0, 0});
        CHECK(patrol.setWaypoints(loop));
        patrol.step(.25f);
        CHECK(close(body.position(), {1, 0, 0}) && patrol.active());
        // Invalid replacement must preserve the current route and loop setting.
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float infinity = std::numeric_limits<float>::infinity();
        const glm::vec3 invalid[] = {{nan, 0, 0}, {0, infinity, 0}, {0, 0, nan}};
        for (const auto &point : invalid)
            CHECK(!patrol.setWaypoints(std::span(&point, 1), false));
        glm::vec3 tooMany[PatrolController::MaxWaypoints + 1]{};
        CHECK(!patrol.setWaypoints(tooMany));
        CHECK(!patrol.setWaypoints({}));
        patrol.step(.25f);
        CHECK(close(body.position(), {0, 0, 0}) && patrol.active());

        JumpCommand jump;
        CHECK(jump.execute(patrol));
        CharacterInputHandler input(patrol);
        input.reset();
        patrol.step(.25f);
        CHECK(!patrol.active() && body.velocity == glm::vec3(0) && body.jumps == 0);
        patrol.start();
        patrol.step(.25f);
        CHECK(close(body.position(), {1, 0, 0}) && patrol.active());

        // Coincident looping points must not spin forever or retain old velocity.
        glm::vec3 coincident[PatrolController::MaxWaypoints]{};
        CHECK(patrol.setWaypoints(coincident));
        patrol.reset({0, 0, 0});
        patrol.step(.25f);
        CHECK(body.velocity == glm::vec3(0) && patrol.active());

        // With direct integration, a route reaches its destination without
        // overshooting at each supported physics cadence.
        for (unsigned rate : {30u, 60u, 120u})
        {
            const glm::vec3 destination[] = {{1, 0, 0}, {1, 0, -1}};
            CHECK(patrol.setWaypoints(destination, false));
            patrol.reset({0, 0, 0});
            for (unsigned tick = 0; tick < rate; ++tick)
            {
                patrol.step(1.0f / rate);
                CHECK(body.position().x >= 0 && body.position().x <= 1.001f);
                CHECK(body.position().z <= 0 && body.position().z >= -1.001f);
            }
            CHECK(!patrol.active());
            CHECK(glm::distance(body.position(), destination[1]) <= PatrolController::ArrivalDistance + .001f);
        }
        return true;
    }
}

int main()
{
    if (!movementChecks() || !patrolChecks())
        return 1;
    std::puts("Controller variants, shared commands and bounded patrol checks passed");
    return 0;
}
