# Gameplay

The gameplay controllers implement the engine's `CharacterController` interface
against a borrowed `CharacterBody`. They have no dependency on the level module,
SDL, rendering or Jolt.

| Controller | Movement policy | Orientation |
| --- | --- | --- |
| `PlayerController` | First-person movement relative to yaw | Look changes yaw and clamped pitch |
| `TopDownController` | Fixed world axes: right is +X, forward is -Z | Look changes aim yaw independently of movement |
| `SideScrollerController` | Left/right movement along world X; ignores forward input | Faces the last nonzero horizontal input; ignores look |
| `PatrolController` | Walks directly between a bounded list of XZ waypoints | Faces the next waypoint; ignores manual move/look |

All four share `GroundController`, an abstract base that handles walk/run speeds,
analog input, diagonal speed limiting, pending jumps and physics stepping.
Jump commands work on every controller. Crouch and attack remain unsupported.
Camera placement and rendering are separate from these movement policies.

`CharacterBody` exposes feet position, reset and stepping with a desired
world-space horizontal velocity and jump request. The body resolves movement,
grounding and collisions and must outlive the controller. A side-scrolling body
must enforce any physical constraint to its plane; the controller restricts
only requested movement.

The level playtest continues to use `PlayerController`. To use top-down movement
with the same body and command bindings:

```cpp
idk::level::Player body; // Initialize against the level's PhysicsWorld first.
idk::gameplay::TopDownController controller(body);
idk::engine::JumpCommand jump;
idk::engine::CharacterInputHandler input(controller);
input.bind(0, &jump);
```

Use `SideScrollerController` the same way. Its `facing()` returns -1 for left
and +1 for right, retaining the last direction when idle. Top-down `yaw()` can
drive aiming independently of movement.

For a character controlled by a patrol route, provide waypoints and advance
the controller each fixed tick:

```cpp
idk::gameplay::PatrolController patrol(body);
const glm::vec3 route[] = {{0, 0, 0}, {4, 0, 0}, {4, 0, -4}};
if (!patrol.setWaypoints(route))
    return false;
patrol.step(fixedDeltaTime);
```

`setWaypoints()` copies up to 32 finite points into `idk::FixedList` and starts
the route. Pass `false` as its second argument to stop at the final point.
Empty, oversized or nonfinite routes return false without changing the old
route. Waypoint height is ignored; the body handles vertical movement.

`clearInput()` stops patrol and cancels pending actions, including when called
by the input handler on capture loss. `start()` restarts at the first waypoint;
`reset(feet)` also resets body position and orientation. Duplicate waypoints
are handled with a bounded scan. An unreachable waypoint remains active:
patrol is direct steering, without navigation or obstacle avoidance. It limits
requested travel near a waypoint; physical braking remains the body's job.

The existing Jolt-backed `level::Player` implements `CharacterBody`. Another
physics backend or a test body can implement the same interface without
changing gameplay code. Level loading and spawn selection remain in the level
module; `Playtest` passes the chosen spawn to `controller.reset()`.

See [character control](../engine/README.md) for input binding and command
examples. All controller state and patrol storage are held in place.

Link `idkgameplay` to use the controllers; it depends only on
`idkengine_character`, GLM and libidk. With `IDK_BUILD_EDITOR_TESTS=ON`, build
`idk_player_controller_tests` and `idk_controller_variants_tests`, then run:

```bash
ctest --test-dir build-cmake/debug/code/gameplay --output-on-failure
```

These checks run against independent bodies without loading a level or physics
backend. They cover shared commands, distinct movement modes, route copying and
validation, looping, stop/restart, duplicate points and physics cadence. The
level tests also exercise the controllers with real Jolt collision.
