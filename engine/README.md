# Character control

## Timing configuration

Edit `engine.cfg` beside the installed executable (normally
`build-debug/engine.cfg`). The file uses `libidk::CfgParser` syntax:

```text
[timing]
{
    fixed_hz = 60
    input_hz = 240
    render_hz = 120
    physics_hz = 120
    vsync = 0
}
```

These are runtime settings: no rebuild is needed. Press **F5** in the game,
shape editor, or map editor to reload the file. Editing alone does not reload
it. Reloads apply between updates, preserve the simulation interpolation phase,
and report success or failure in the console. Invalid syntax, unknown timing
keys, out-of-range rates, or a failed VSync change leave all active settings
intact.

| Key | Default | Meaning |
| --- | --- | --- |
| `fixed_hz` | `30` | Simulation callbacks per second, from 4 to 1000 |
| `input_hz` | `0` | Input/`onUpdate` polls per second, from 1 to 1000; 0 follows rendering |
| `render_hz` | `0` | Render FPS cap, from 1 to 1000; 0 is uncapped |
| `physics_hz` | `120` | Minimum player physics substep frequency, from 30 to 1000 |
| `vsync` | `1` | Swap synchronization: 0 off, 1 on |

Installation creates the default file only when absent, preserving local edits
on later builds. Missing keys use the defaults above on every load, including
reloads. A missing default file at startup uses built-in defaults; an explicitly
selected file must exist. Invalid files fail startup with a nonzero exit code.
Other sections may coexist with `[timing]`. Use `#` for comments.

Select a different file with `--config` in any of the three applications:

```bash
./build-debug/game --config ./my-settings.cfg \
    --level build-debug/content/level/playground.idklvl
```

The file path is resolved before the engine changes working directory and is
reused for each F5 reload. **Build & Play** forwards the map editor's config
path to the child game. Each running application reloads independently.
The earlier timing environment variables are replaced by these file settings.

Scheduling is single-threaded: disable VSync to poll faster than the
display refresh rate. Slow callbacks, rendering, and OS scheduling can still
reduce achieved rates. Missed input polls and render frames are skipped;
fixed simulation ticks catch up. Keyboard taps entirely between polls may
still be missed.

`onUpdate` now runs at the input rate, including event pumping and other
variable-update services. `getDeltaTimeSec()` reports elapsed input time there
and elapsed render time in render callbacks. Explicit
`getInputDeltaTimeSec()` and `getRenderDeltaTimeSec()` accessors are also
available. Physics uses `getFixedDeltaTimeSec()`, and render interpolation
continues to use `getFixedUpdateAlpha()`.

Programmatic configuration uses `EngineTimingConfig` and
`Engine::setTimingConfig(config)` before `startApplication()`. The setter
returns false for invalid settings or after startup. It replaces the full
configuration, including file settings. `setFixedUpdateRate(hz)`
remains available and changes only the fixed rate. Services can inspect the
effective configuration through `EngineAPI::getTimingConfig()` and trigger a
manual reload with `EngineAPI::requestTimingReload()`. Pass an optional path
to `Engine(configPath)` to select a file from code; `getTimingConfigPath()`
on the API returns that resolved path. A reload replaces programmatic overrides
with defaults plus the current file. Custom
`level::Player` owners can call `setPhysicsRate(hz)`; the playtest applies
`physicsRateHz` automatically on initialization and every fixed update.

Each player update subdivides its full fixed interval into equal steps no
larger than approximately `1 / physicsRateHz`. Lower physics substep rates
can change acceleration and collision accuracy. Both the engine and player cap
a single elapsed interval at 250 ms, which also sets the minimum
supported fixed rate of 4 Hz.

## Character input

`CharacterController` is the device- and physics-independent interface for
movement, look and actions. `CharacterCommand` objects send action requests to
that interface. `CharacterInputHandler` maps button press/release edges to
commands; it does not poll SDL or advance physics.

`CharacterInput` is a frame snapshot with movement axes, look deltas in
degrees, a sprint state and 16 caller-assigned button slots. Bindings borrow
commands, so keep the controller and commands alive longer than the handler:

```cpp
using namespace idk::engine;

// character is a game-specific CharacterController implementation.
JumpCommand jump;
CrouchCommand crouch(true), stand(false);
AttackCommand attack;
CharacterInputHandler input(character);
input.bind(0, &jump);
input.bind(1, &crouch, &stand);
input.bind(2, &attack);

// Once per input frame: populate these values from a keyboard, gamepad or AI.
CharacterInput frame;
frame.moveY = 1.0f;
frame.buttons[0] = jumpButtonDown;
frame.buttons[1] = crouchButtonDown;
frame.buttons[2] = attackButtonDown;
input.update(frame);

// Separately, once per fixed physics tick:
character.step(fixedDeltaTime);
```

The handler applies look before movement and executes commands only when a
button changes state. Controllers retain pending actions until their next
`step()`, so a press is not lost when a render frame has no physics tick. Held
buttons do not repeat press commands. Crouch uses separate press/release commands
to request crouching and standing. Command execution returns whether the
controller accepted the request; input dispatch ignores unsupported actions.
Call a command's `execute(character)` directly when an AI or script needs that
result or should request an action without a button edge.

Call `input.reset()` when input capture is lost. It sends release commands,
clears button history and cancels controller intent. To respawn while preserving
held-button history, reset the concrete controller instead. `bind()` rejects
out-of-range or currently held slots; release/reset first when rebinding.
Passing null command pointers unbinds the corresponding edges.

Implement `CharacterCommand` for additional actions and bind any free slot.
Override optional `jump()`, `crouch(bool)` and `attack()` methods in a controller
to support those requests; the defaults return false. No command or binding
requires heap allocation, exceptions, lambdas or `std::function`.

`CharacterBody` is the physics-facing interface used by gameplay controllers.
It exposes position, reset and movement stepping without referencing a level or
physics backend. The [gameplay module](../gameplay/README.md) provides first-person,
top-down, side-scrolling and patrol controllers against this interface. The
level playtest connects the first-person controller to the Jolt-backed
`level::Player`. Crouch and attack commands are available to gameplay controllers,
but those mechanics are not implemented by the level playtest.

The `idkengine_character` CMake target uses GLM for body coordinates and has no
gameplay, level, SDL, renderer or Jolt dependencies.
With `IDK_BUILD_EDITOR_TESTS=ON`, build `idk_character_tests`,
`idk_timing_tests`, and `idk_timing_reload_tests`, then run:

```bash
ctest --test-dir build-debug/cmake/engine --output-on-failure
```

The reload test uses offscreen OpenGL 4.6 and checks the real F5 event path,
explicit API requests, no automatic reload, and preservation on failure.
