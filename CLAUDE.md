# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Fort Candle is a small low-poly strategy game (RWTH Aachen GameDev practical 2021) built on a custom engine that is tightly coupled to the game. C++17, OpenGL 4.5, CMake. The CMake project/executable name is `GameDevSS21`. All dependencies are vendored in `extern/` (glfw, glow, glow-extras, imgui, polymesh, typed-geometry, clean-core, openal, dr-lib); treat them as third-party and don't modify them.

There are no tests, linters, or CI. "Verifying" a change means building and, where relevant, running the game.

## Build & run

```powershell
cmake -S . -B build
cmake --build build --config RelWithDebInfo --parallel
```

- Requires CMake ≥ 3.21 and is tested with CMake 4.4 and MSVC 19.51 (Visual Studio 2026). Most `extern/` projects still declare `cmake_minimum_required(VERSION 3.0)`, which CMake 4 rejects; the top-level `CMakeLists.txt` sets `CMAKE_POLICY_VERSION_MINIMUM` around their `add_subdirectory` calls instead of patching them.
- Single-config generators (Ninja, Makefiles) default to `CMAKE_BUILD_TYPE=RelWithDebInfo`. Multi-config generators (Visual Studio) pick the config at build time, and a plain `cmake --build build` builds **Debug**. Debug builds are *much* slower at runtime.
- The executable and `OpenAL32.dll` are written to `bin/<Config>/` (e.g. `bin/RelWithDebInfo/GameDevSS21.exe`). In Visual Studio, `GameDevSS21` is the startup project and its debugger working directory is already `bin/`.
- `bin/`, `build/`, and `build_vs/` are gitignored.
- The game target compiles with `/W3 /MP /utf-8` on MSVC (keep sources UTF-8) and `-Wall -Wno-unused-variable` elsewhere. `stdc++fs` is linked only for GCC < 9.1.
- Linux (checked on Ubuntu 26.04 in WSL2 with GCC 15.2 / Clang 21.1 / CMake 4.2):
  - Packages: `libgl-dev`, `libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev`, and `libpulse-dev` and/or `libasound2-dev` (without one of them OpenAL has no audio backend). GLFW is built X11-only; Wayland sessions use XWayland.
  - **`extern/` does not compile unpatched with GCC ≥ 14 / libstdc++ ≥ 13**: missing `#include <cstdint>` in `glow/common/log.hh` and `glow-extras/colors/color.hh`, `always_false<D>` with an `int` parameter in typed-geometry (7 places), `this->mValue` in OpenAL's `common/aloptional.h`, and hard-coded `-fuse-ld=gold` in `clean-core/cmake/CompileOptions.cmake` and `polymesh/CMakeLists.txt` (gold is gone from binutils ≥ 2.44).
  - The top-level `CMakeLists.txt` sets `GLOW_USE_GOLD_LINKER=OFF` and `GLOW_ENABLE_MARCH_NATIVE=OFF` (portable binaries; `-march=native` also enabled a broken AVX-512 path in glow's bundled xxHash).
  - The game links `OpenGL::GL` with `--no-as-needed`. Without that, libGL is only loaded by GLFW's `dlopen`, `glfwTerminate()` unloads it, and `~Game()` then crashes (SIGSEGV) while deleting GL objects.
  - `glow`, `glow-extras` and `glad` are shared libraries on Linux; they land next to the executable in `bin/<Config>/` and are found via RPATH.
  - The OpenAL JACK backend is off by default (`ALSOFT_BACKEND_JACK=OFF`) because it failed to link with `undefined symbol: jack_error_callback`.

**Working directory matters.** `main.cc` aborts unless `../data` exists, and every asset path in the code is hard-coded relative (`../data/meshes/`, `../data/shaders/`, `../data/config/`, ...). Run the game from a direct subfolder of the repo root, e.g. from `bin/` (`./RelWithDebInfo/GameDevSS21`) or from `build/` (`../bin/RelWithDebInfo/GameDevSS21`).

Code style: `.clang-format` (Google-based, Allman braces, 4-space indent, 150 column limit). Engine code lives in `namespace gamedev`; members use an `m` prefix.

## Architecture

### Entry point and main loop

`main.cc` → `Game` (`src/Game.hh/.cc`), which subclasses glow-extras' `glow::glfw::GlfwApp`. `GlfwApp` drives the loop: `update()` runs at a fixed 60 Hz, `render()` once per frame, and `onGui()` builds the ImGui UI (menus, HUD, debug and editor panels). `Game` owns everything: the ECS, every system and factory, the terrain, the camera, and the render settings. Most of the wiring lives in `Game::init()`.

### ECS (`src/ecs/`)

The ECS is based on austinmorlan.com's ECS article. `EngineECS` (`ecs/Engine.hh`) is the facade the rest of the code talks to. It wraps the `ComponentManager`, `SystemManager`, `EventManager`, the `World`, and the spatial hash map (`Hash2D`).

- **Entities are `InstanceHandle`s**, managed by `World` (`advanced/World.hh`). An `Instance` is not a pure ID: it directly holds the render data (VAO, albedo/normal/ARM textures, `transform xform`, bounds, auxiliary models, a simple animation type) plus the component `Signature` bitset and a `destroy` flag.
- **Limits** are in `Constants.hh`: `MAX_COMPONENTS = 32`, `MAX_INSTANCES = 2048`.
- **Components** (`src/components/`, all included through `Components.hh`) derive from `Component`, which keeps a back-pointer to its `Instance` and its `handle_value`. Several component structs can share one header, e.g. `UnitComp.hh` defines `Living`, `Mortal`, `Attacker`, etc. Component and system types are keyed by `typeid(T).name()`.
- **Systems** (`src/systems/`) derive from `System` and receive every entity whose signature contains the system's signature, collected in `mEntities` (a `std::set<InstanceHandle>`). Systems can override `AddEntity`/`RemoveEntity` to keep their own containers.

Adding a new component or system requires matching edits in `Game.cc`/`Game.hh`:
1. `mECS->RegisterComponent<T>()` in `Game::init()`. Using a component before it is registered asserts.
2. `mECS->RegisterSystem<S>()`, then build a `Signature` from `GetComponentType<...>()` and call `SetSystemSignature<S>()`.
3. Call the system's `Init(mECS, ...)`, and call its `Update(dt)` from `Game::update()`. `Update` returns elapsed microseconds, which feed the debug timing UI.

Notes:
- **`Prototype` entities are never added to systems** (`EngineECS::UpdateSystems`). Factory presets carry this component, so they exist only as templates to clone.
- **Destruction is deferred.** Call `mECS->MarkDestruction(handle)`, and `CleanupSystem` destroys marked instances at the end of the update. That removes them from all component arrays, all systems, and the spatial hash.
- **Events** (`ecs/Event.hh`) have a fixed `EventType` enum (collisions, hits, deaths, supply upgrades, ...). `SendFunctionalEvent` dispatches synchronously to listeners registered with `AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::X, Class::Method))`. `SendEvent` only queues events into `GetPendingEvents()` for manual handling. Collision results reach gameplay this way, from `CollisionSystem` to `GamelogicSystem` listeners. Moving an entity should send `EventType::NewPosition` so `CollisionSystem` updates its collider hash maps.

### Systems

- `GamelogicSystem` is by far the largest file (~2.5k lines). It holds all gameplay: resources, building, unit AI and combat, enemy waves, and win/lose state. It also drives some render parameters (desaturation and vignette radius come from game state). It is initialized with references to the audio system and the unit, structure, and item factories.
- `CollisionSystem` handles 2D-only collision (OBB vs. circle shapes on the ground plane) and uses `Hash2D` for broad-phase.
- `RenderSystem` (`AdvancedRenderSystem.cc`) is a forward renderer. Pass order: point-light depth cubemaps → directional shadow map → skybox → scene → particles → post-processing (outlines, bloom, tonemap, sepia, vignette, distance fade). Shaders are loaded from `data/shaders/` by base name: `Program::createFromFile("../data/shaders/object")` picks up `object.vsh` + `object.fsh`. Shader includes use `.glsli`.
- The other systems are `PhysicsSystem`, `EnvironmentSystem` (day/night game time), `LightSystem`, `ParticleSystem` (instanced, plus a compute shader), `AnimationSystem` (non-skeletal), `AudioSystem` (OpenAL 3D audio; sound files in `data/audio/`), and `CleanupSystem`.

### Assets and factories

- `GameObjects` (`SharedGameObjects`) is the asset manager. `scanMeshes()` recursively registers every `.obj` under `data/meshes/`, and assets can then be looked up by filename (without extension) or by full path. All models share one palette texture (`data/textures/BigPalette.png`, low-poly flat-color style). `models/` holds the Blender source files, which are not loaded at runtime.
- `Factory` (`src/Factory.hh`) manages named presets: prototype entities that `Create(name, pos, rot, scale)` clones. The subclasses `UnitFactory`, `StructureFactory`, `NatureFactory`, and `ItemFactory` override registration to attach the right components. In `Game::init()`, each factory auto-registers the meshes in its `data/meshes/<folder>/` (`units`, `structures`, `nature`, `items`) with a per-factory scale.
- **World layouts are `.fac` files** in `data/config/`. Each is a binary sequence of `[null-terminated preset name][vec3 translation][quat rotation][size3 scaling]` records. They are loaded at startup (`nature.fac`, `items.fac`, `startStructs.fac`) and by gameplay (e.g. the `supplyroute_*.fac` upgrades). They are written by the in-game editor (toggle with **E**), which spawns presets and saves either all objects or only the newly created ones under a chosen filename. Don't hand-edit these files; regenerate them through the editor.

### Other directories

- `src/advanced/` contains the instance `World`, `transform`, the stats/debug helpers, and the editor. It originated from course framework code.
- `src/utility/` contains the spatial hash maps, a quadtree, RNG, and sound loading (dr_wav/dr_mp3).
- `src/types/` contains plain data types shared across components (lights, materials, particles, unit and structure properties).
- `docs/` contains early design notes and a ToDo list from the start of the project. They are not authoritative about the current code.
