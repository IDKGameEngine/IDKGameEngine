# IDKGameEngine

The engine, its build configuration, and shared assets live together in this
repository. `code/` contains the engine modules and their source, headers,
tests, and compiled shader source. `data/content/` contains installable runtime
assets; `data/examples/` contains editable sample maps and models; and
`data/tests/` contains test fixtures. `thirdparty/` contains pinned dependency
submodules and the local `cgltf` header. Generated builds and installed
dependencies stay in ignored `build-cmake/`, `build-debug/`, `build-release/`,
and `.local/` directories.

`libidk` and the optional demo game remain independent projects. By default,
place them beside this checkout as `../libidk` and `../idk_game`. To use other
locations, pass `-DIDK_LIBIDK_DIR=/absolute/path` and
`-DIDK_GAME_DIR=/absolute/path` through `./scripts/build.sh --cmakeopts ...`.
The engine and editors build without the demo game; **Build & Play** in the map
editor requires an installed game executable.

[Networking](code/network/README.md) provides server-authoritative entities,
client snapshots and resource-ID mapping over libidk's `RemoteRxTxer`, with a
headless server/client demo and a graphics-free `IDK_NETWORK_ONLY` build.

Build inside the Steam Linux Runtime container:

```bash
git submodule update --init --recursive
./scripts/env.sh
./scripts/dep-build.sh
./scripts/build.sh
```

`dep-build.sh` builds dependencies into `.local/`. An existing compatible
`.local/` installation can be reused; rebuild individual packages with, for
example, `./scripts/dep-build.sh vulkan-sdk`.
`./scripts/build.sh --preset=release` builds a release version. For tests,
configure with `./scripts/build.sh --cmakeopts -DIDK_BUILD_EDITOR_TESTS=ON`,
then run
`ctest --test-dir build-cmake/debug --output-on-failure` inside the container.
After building, `./scripts/package.sh --preset=debug` creates
`idk-linux-x86_64-debug.tar.gz` from the installed files in `build-debug/`.
The CMake working files stay in `build-cmake/debug/` (or
`build-cmake/release/` for a release build).

Runtime timing settings are in `build-debug/engine.cfg` after installation.
Edit the file and press **F5** in a running application to reload it without
rebuilding. See [engine timing configuration](code/engine/README.md#timing-configuration).

The default installed executables are `build-debug/game`,
`build-debug/editor`, `build-debug/mapedit`, and `build-debug/mapc` when the
demo game is available. Engine modules and tools have their own README files.
