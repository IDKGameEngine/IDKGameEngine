# IDKGameEngine

The engine, its build configuration, and shared runtime assets live together
in this repository. `asset/`, `common/`, `editor/`, `engine/`, `gameplay/`,
`gfx/`, `level/`, and `mapedit/` are the engine modules. `content/` contains
installable assets; `thirdparty/` contains pinned dependency submodules and
the local `cgltf` header. Generated builds and installed dependencies stay in
ignored `build-debug/`, `build-release/`, and `local/` directories.

`libidk` and the optional demo game remain independent projects. By default,
place them beside this checkout as `../libidk` and `../idk_game`. To use other
locations, pass `-DIDK_LIBIDK_DIR=/absolute/path` and
`-DIDK_GAME_DIR=/absolute/path` through `./build.sh --cmakeopts ...`.
The engine and editors build without the demo game; **Build & Play** in the map
editor requires an installed game executable.

Build inside the Steam Linux Runtime container:

```bash
git submodule update --init --recursive
./env.sh
./dep-build.sh
./build.sh
```

`dep-build.sh` builds dependencies into `local/`. An existing compatible
`local/` installation can be reused; rebuild individual packages with, for
example, `./dep-build.sh vulkan-sdk`. `./build.sh --preset=release` builds a
release version. For tests, configure with
`./build.sh --cmakeopts -DIDK_BUILD_EDITOR_TESTS=ON`, then run
`ctest --test-dir build-debug/cmake --output-on-failure` inside the container.

Runtime timing settings are in `build-debug/engine.cfg` after installation.
Edit the file and press **F5** in a running application to reload it without
rebuilding. See [engine timing configuration](engine/README.md#timing-configuration).

The default installed executables are `build-debug/game`,
`build-debug/editor`, `build-debug/mapedit`, and `build-debug/mapc` when the
demo game is available. Engine modules and tools have their own README files.
