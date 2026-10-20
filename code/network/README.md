# Networking

`idkengine_network` provides a server-authoritative world and client replicas
over libidk's `RemoteRxTxer` (SDL3_net UDP). It runs without a window, renderer,
audio device, Jolt or Steamworks. The same protocol handles local singleplayer
and remote multiplayer. `idk_net_demo` demonstrates both. The sibling
[`idk_game`](../../../idk_game/README.md) integrates this into its model demo
and compiled-level game, including private server processes, dedicated hosting,
authoritative Jolt players and synchronized level transitions. The standalone
engine `level::Playtest` service remains available for local tools/tests.

## Build and run

On Linux with SDL3, SDL3_net, GLM, CMake and Ninja installed:

```bash
cmake -S . -B build-network -G Ninja \
    -DIDK_NETWORK_ONLY=ON -DCMAKE_PREFIX_PATH="$PWD/.local" \
    -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build-network
ctest --test-dir build-network --output-on-failure

# Local server and client, separate worlds, communicating over loopback UDP.
./build-network/code/network/idk_net_demo local 28080 300

# Dedicated headless server, in its own process (3000 simulation ticks).
./build-network/code/network/idk_net_demo server 28080 3000

# In another terminal; run additional clients to see multiple entities.
./build-network/code/network/idk_net_demo client 127.0.0.1 28080 300
```

For remote multiplayer, replace `127.0.0.1` with the server hostname. The demo's
`server` mode listens on all interfaces; `local` binds only loopback. The public
`Server::listen` API defaults to loopback; pass `nullptr` as its third argument
to listen on all interfaces. Ports must be nonzero. Clients bind independent
ephemeral ports, so many can run on the same machine. The demo runs at 30 Hz,
creates a moving object per connected client, and prints replicated positions.
Ctrl+C exits early. The `local` example does not launch a subprocess: use the
two commands above when you want a separate dedicated server for singleplayer.

The normal engine build includes the networking library, demo and tests too.
`IDK_NETWORK_ONLY` skips graphics/physics/Steamworks dependency discovery.
Session tokens currently use Linux `getrandom`; other operating systems need
an entropy-provider implementation. SDL_net handles the transport itself.

## Ownership and lifetime

| Data | Owner | Network representation |
| --- | --- | --- |
| Simulation objects, physics, gameplay rules | Server | Explicit replicated state |
| Entity identity | Server | Slot plus generation (`EntityId`) |
| Player input | Client proposes; server validates | Axes and held button states |
| Models, materials, audio, GPU handles | Each process | Stable `ResourceId` only |
| UI, camera, particles and other presentation | Client | Normally not replicated |

`Server::world()` owns a fixed pool. `spawn(owner, resource, transform)` returns
zero on invalid data or exhaustion. `destroy(id)` and `setTransform(id, value)`
reject stale IDs. Reusing a slot increments its generation; exhausted
generation slots are retired. `clear()` and server restarts on the same object
preserve generation history. Entity IDs are scoped to a session, not persistent
save-game IDs. Owner zero means server-owned; use a confirmed `clientAt(slot)`
ID for player-controlled entities. Peer IDs are not reused on reconnect.

Client snapshots are read-only through the client API. A snapshot replaces
the entire replicated world atomically: a newly present ID is a spawn, a missing
ID is a despawn, and a surviving ID updates its state. Compare full IDs, including
generation. Late joiners receive the current world on the next publication.
Disconnect/timeout removes that client's server-owned player entities. Client
disconnect, rejection, transport failure or timeout clears both snapshots.
The application must likewise release any corresponding presentation objects.

References returned by `World::find` or `at` can change after world mutation.
Snapshot references change on the next accepted snapshot; copy values you need
to retain. Never store a replicated array index as a permanent identity.

Resources are intentionally not automatically loaded from network data. Assign
stable nonzero IDs in your content build and pass the same nonzero manifest
fingerprint to `listen` and `connect`. Mismatches are rejected. The fingerprint
is supplied by the application; this module does not generate or transfer a
manifest. Bind those IDs to already loaded resources on each client:

```cpp
idk::net::ResourceMap<idk::gfx::ModelHandle, 16> models;
models.bind(1, playerModel); // playerModel was loaded locally.
for (const auto &entity: std::span(client.snapshot().entities, client.snapshot().count))
{
    if (const auto *model = models.find(entity.resource))
    {
        // Submit *model with entity.transform converted to your render matrix.
    }
}
```

An unresolved resource ID should produce an application-defined placeholder or
an error. It must not become an arbitrary path to load. Servers only need
simulation resources such as collision data, not GPU assets. Shared immutable
assets may be reused within one process; authoritative objects and replicas
remain separate even in local mode.

## Integrating a game

The application owns sessions and calls them on one thread. Supply monotonic
seconds from `nowSeconds()` to `poll`; nonfinite or backwards timestamps fail
without changing session state. Opening a session can wait up to one second
per DNS resolution. Polling is nonblocking and examines at most 64 datagrams
per call. Application-level peer, world, input and packet storage is fixed;
SDL_net still allocates its own socket, address and datagram objects.

Server loop:

1. Call `server.poll(nowSeconds())` frequently to handle connections and input.
2. Create game objects for newly confirmed IDs from `clientAt`.
3. Once per fixed simulation tick, sample `server.input(entityId, input)`.
   Advance the authoritative controller/physics using the **server's** timestep
   and gameplay limits; write the result with `world().setTransform`.
4. Call `server.publish(tick)` after simulation, even if the world is empty.

Client loop:

1. Call `client.poll(nowSeconds())` frequently and inspect `status()`.
2. Once connected and its owned entity is present, call `sendInput` regularly
   with the current intent. Resend held states even when unchanged.
3. Reconcile presentation objects against `snapshot()`. `previousSnapshot()`
   and the server tick are available for application interpolation. Interpolate
   only matching IDs; do not interpolate across a destroyed/reused slot.

`InputState` has three axes in `[-1, 1]` and 32 application-defined held button
bits. The server checks session, packet sequence, entity generation, ownership
and finite/ranged axes before storing input. A newer packet replaces the latest
intent; receiving more packets does not run more simulation steps. Input becomes
neutral after 250 ms without an accepted update. The game must enforce diagonal
speed, movement modes, collision and action cooldowns. The demo shows normalized
movement with a server-selected timestep. One-shot actions such as inventory
transactions need a separate acknowledged command protocol; a brief button tap
can be lost on UDP. This layer does not promise reliable events.

For the existing engine service loop, `idk::NetService` polls a borrowed session:

```cpp
idk::net::Client client;
if (!client.connect("127.0.0.1", 28080, contentManifest, idk::net::nowSeconds()))
    return false;
idk::NetService networking(client);
engine.addService(&networking); // Before startApplication(&game).
```

Include `idk/engine/NetService.hpp`. Keep both objects alive through engine
shutdown. The service polls in `onUpdate` and disconnects/stops in `onShutdown`;
it does not create a session or quit the application on a connection failure.
Its server overload accepts `Server &`. Publication is explicit so the game can
send snapshots **after** its fixed update. Headless applications use the sessions
directly instead of constructing `Engine`, which initializes graphics services.

## Protocol and limits

The transport envelope is 32 bytes: four 8-byte fields (protocol marker/version,
total size, payload size, payload type). Integers are big-endian; type is eight
raw ASCII bytes. Engine messages use type `IDKNET1`, with a 25-byte header
(kind, client nonce, server session token, sequence). Transforms use explicit
IEEE-754 binary32 fields. No pointers, padding, native structs or resource
handles cross the wire. This replaces the previously disabled transport's
native-layout format; old binaries are not wire-compatible.

Hello/welcome retries and heartbeat replies maintain sessions. A client must
return the server token before becoming visible as a confirmed player. Sessions
expire after five seconds without valid traffic. Explicit disconnect is best
effort, with timeout as fallback. Client reconnect is explicit via `connect()`;
it clears replicas and uses a fresh nonce/socket. Packets from other endpoints,
old sessions, duplicate/out-of-order sequences, malformed lengths, nonfinite
state, duplicate entity slots and unsupported envelope versions are rejected.

Limits are **8 clients and 16 replicated entities**. A complete snapshot fits
inside the 1024-byte transport payload (771 bytes on the wire at capacity),
avoiding application fragmentation. These are a bounded starting point, not a
large-world replication system. Increasing entity capacity requires pagination
with atomic assembly or interest management; a compile-time check prevents
silently exceeding the payload. Full snapshots heal packet loss but cost more
bandwidth than delta replication. The module does not implement encryption,
account authentication, NAT traversal, reliable RPCs, prediction/reconciliation,
or replication of arbitrary gameplay components. Tokens distinguish sessions;
they are not a replacement for an authenticated encrypted transport.

Tests cover byte order and bounds, two-client routing, malformed transport
packets, resource mapping, pool exhaustion, generations, admission limits,
manifest mismatch, ownership, replay/reordering, dropped snapshots, timeout,
reconnect, source validation and atomic snapshot rejection. They require
permission to open local UDP sockets. SDL_net behavior follows its documented
[datagram socket](https://wiki.libsdl.org/SDL3_net/NET_CreateDatagramSocket) and
[receive/lifetime](https://wiki.libsdl.org/SDL3_net/NET_ReceiveDatagram) contracts.
