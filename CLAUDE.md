# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Fort Candle is a small low-poly strategy game (RWTH Aachen GameDev practical 2021) built on a custom engine that is tightly coupled to the game. C++17, OpenGL 4.5, CMake. The CMake project/executable name is `GameDevSS21`. All dependencies are vendored in `extern/` (glfw, glow, glow-extras, imgui, polymesh, typed-geometry, clean-core, openal, dr-lib); treat them as third-party. Prefer workarounds in the top-level `CMakeLists.txt` or the game; if a library must change, do it in a separate commit per library named `extern/<lib>: ...` (see [Vendored libraries](#vendored-libraries-extern)).

There are no tests, linters, or CI. "Verifying" a change means building and, where relevant, running the game.

## Build & run

```powershell
cmake -S . -B build
cmake --build build --config RelWithDebInfo --parallel
```

- Requires CMake ≥ 3.21 and is tested with CMake 4.4 and MSVC 19.52 (Visual Studio 2026). Most `extern/` projects still declare `cmake_minimum_required(VERSION 3.0)`, which CMake 4 rejects; the top-level `CMakeLists.txt` sets `CMAKE_POLICY_VERSION_MINIMUM` around their `add_subdirectory` calls instead of patching them.
- Single-config generators (Ninja, Makefiles) default to `CMAKE_BUILD_TYPE=RelWithDebInfo`. Multi-config generators (Visual Studio) pick the config at build time, and a plain `cmake --build build` builds **Debug**. Debug builds are *much* slower at runtime.
- The executable and `OpenAL32.dll` are written to `bin/<Config>/` (e.g. `bin/RelWithDebInfo/GameDevSS21.exe`). In Visual Studio, `GameDevSS21` is the startup project and its debugger working directory is already `bin/`.
- `bin/`, `build/`, and `build_vs/` are gitignored.
- The game target compiles with `/W3 /MP /utf-8` on MSVC (keep sources UTF-8) and `-Wall -Wno-unused-variable` elsewhere. `stdc++fs` is linked only for GCC < 9.1.
- Linux (checked on Ubuntu 26.04 in WSL2 with GCC 15.2 / Clang 21.1 / CMake 4.2):
  - Packages: `libgl-dev`, `libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev`, and `libpulse-dev` and/or `libasound2-dev` (without one of them OpenAL has no audio backend). GLFW is built X11-only; Wayland sessions use XWayland.
  - **The 2021 `extern/` snapshots do not compile unpatched with GCC ≥ 14 / libstdc++ ≥ 13 and binutils ≥ 2.44.** The six fixes are applied as one commit per library: `4797612` glow and `e983060` glow-extras (missing `#include <cstdint>` in `common/log.hh` / `colors/color.hh`), `6ec1160` typed-geometry (`always_false<D>` with an `int` parameter, 7 places), `a90ca57` OpenAL (`this->mValue` in `common/aloptional.h`), `b66bf1e` clean-core and `69db695` polymesh (hard-coded `-fuse-ld=gold` in `cmake/CompileOptions.cmake` / `CMakeLists.txt`; gold is gone from binutils ≥ 2.44). MSVC does not need them. Current upstream versions no longer need them (see [Vendored libraries](#vendored-libraries-extern)).
  - The top-level `CMakeLists.txt` sets `GLOW_USE_GOLD_LINKER=OFF` and `GLOW_ENABLE_MARCH_NATIVE=OFF` (portable binaries; `-march=native` also enabled a broken AVX-512 path in glow's bundled xxHash).
  - The game links `OpenGL::GL` with `--no-as-needed`. Without that, libGL is only loaded by GLFW's `dlopen`, `glfwTerminate()` unloads it, and `~Game()` then crashes (SIGSEGV) while deleting GL objects.
  - `glow`, `glow-extras` and `glad` are shared libraries on Linux; they land next to the executable in `bin/<Config>/` and are found via RPATH.
  - The OpenAL JACK backend is off by default (`ALSOFT_BACKEND_JACK=OFF`) because it failed to link with `undefined symbol: jack_error_callback`. The cause is the vendored `ALSOFT_DLOPEN=OFF` default (see below): without dlopen, JACK symbols are linked directly instead of being looked up as optional.

**Working directory matters.** `main.cc` aborts unless `../data` exists, and every asset path in the code is hard-coded relative (`../data/meshes/`, `../data/shaders/`, `../data/config/`, ...). Run the game from a direct subfolder of the repo root, e.g. from `bin/` (`./RelWithDebInfo/GameDevSS21`) or from `build/` (`../bin/RelWithDebInfo/GameDevSS21`).

Code style: `.clang-format` (Google-based, Allman braces, 4-space indent, 150 column limit). Engine code lives in `namespace gamedev`; members use an `m` prefix.

Documentation: Doxygen comments, written to be read in the code first (the generated docs come second). The top-level `Doxyfile` covers `src/` (without `dr_wav`) and lists only non-default settings. `cmake --build build --target docs` (defined only if CMake finds Doxygen) writes HTML to `build/docs/html`, as does `doxygen Doxyfile` from the repo root. Check that a docs change still builds without Doxygen warnings.
- Multi-line docs use `/** ... */` blocks with a ` * ` prefix per line; single-line docs use `///`, data members and enumerators a trailing `///<` (or `///` above the member if the line would exceed 150 columns). Align consecutive `///<` like clang-format's `AlignTrailingComments`.
- Line breaks follow the text, not the column limit. A paragraph, list item or tag description that fits in about 120 columns stays on one line. A longer one starts each sentence on a new line, and a sentence that does not fit in 150 columns breaks at a clause boundary (after a comma, semicolon or colon, or before a parenthesis), never mid-phrase.
- Within a block: if the first paragraph has more than one sentence and does not fit on one line, the first sentence becomes the summary line, followed by an empty ` *` line. Lists are written ` *  - item` / ` *  1. step`, with continuation lines aligned to the item text; `@param`/`@return` continuation lines align with the description. A `// Label` comment that heads a group of documented declarations is followed by an empty line.
- Class docs explain purpose, ownership and how the class fits into the ECS. Give them structure: a one-line summary, then short paragraphs and `-`/`1.` lists separated by empty ` *` lines.
- Function docs state intent and effect (side effects, ownership, units, coordinate spaces) in as few words as possible. They live at the declaration (header). Separate documented declarations by an empty line.
- Functions that are not plain getters, setters or operators get a `/** ... */` block: a brief line, short details if they help (each its own paragraph), then `@param` for every parameter (`@param[out]`/`@param[in,out]` for output parameters) and `@return` for a non-void result. Getters, setters, operators and parameterless functions whose brief says it all keep a one-line `///`.
- `.cc` files only get plain `//` comments for implementation details.
- Known bugs and quirks start with `TODO:`, in the doc block (as its own paragraph) and/or as `// TODO:` at the code site. Keep existing `TODO`/`ToDo` markers until resolved.
- Code that exists but never runs is marked `Unused:`. For a class, it is its own paragraph right after the summary (`Unused: <reason>.`). For a member, enumerator, function or parameter, its doc ends with `Unused.`, or `Unused: <reason>.` if the reason is not obvious. If a decision is pending (finish, wire up or remove), use `TODO:` instead, and `@deprecated` only for code that is going to be removed.
- Don't keep commented-out code; git history has it. Exceptions: the unfinished `QuadTree` (`utility/QuadTree.*`) and `GPUParticleSystem` (end of `ParticleSystem.*`), kept with a note.

## Vendored libraries (`extern/`)

Everything in `extern/` was imported as plain files in one commit (`1d74651`, 2022-01-19), without upstream metadata. The snapshots below were identified by matching file contents against upstream history. The arcana/Glow libraries have no maintained release tags (`develop` is their only active branch), so pin commits.

| Directory | Upstream | Snapshot | Local differences |
|---|---|---|---|
| `clean-core` | https://github.com/project-arcana/clean-core | develop `8e2eba0` (2021-03-11) | `b66bf1e`; CRLF-only diffs in 3 `native/win32_*` files |
| `typed-geometry` | https://github.com/project-arcana/typed-geometry | develop `9b48a94` (2021-03-01) | `6ec1160`; stale `detail/sum_of_pos.hh` |
| `polymesh` | https://github.com/project-arcana/polymesh | develop `fba09a7` (2021-01-25) | **OBJ reader patch** (below); `69db695`; stale `algorithms/barycoords.hh` |
| `glow` (bundles `extern/glad`) | https://gitlab.vci.rwth-aachen.de:9000/Glow/glow | develop `8f61a37` (2021-04-05) | `4797612`; an unused `struct transform;` in `fwd.hh` |
| `glow-extras` | https://gitlab.vci.rwth-aachen.de:9000/Glow/glow-extras | develop `366c039` (2021-02-20) | **`GlfwApp` patch** (below); `e983060` |
| `imgui` | `imgui-lean` wrapper fork: https://gitlab.vci.rwth-aachen.de:9000/ptrettner/imgui-lean | `fcf73e7` (2020-04-17) | none. Bundles imgui 1.77 WIP, GLFW/OpenGL3 backends from 2018 and ImGuizmo ≈ 1.61 WIP |
| `glfw` | https://github.com/glfw/glfw | master `2c7f3ce9` (2021-02-17, a 3.4.0 pre-release) | 21 stale files from older GLFW versions (e.g. `deps/glad.c`, `deps/vulkan/`) |
| `openal` | https://github.com/kcat/openal-soft | tag `1.21.1` (`ae4eacf14`) | `a90ca57`; **CMake option defaults** (below) |
| `dr-lib` | https://github.com/mackron/dr_libs | dr_wav 0.12.19, dr_mp3 0.6.27, dr_flac 0.12.29 | not used by the build; the game compiles its own identical copy `src/utility/dr_wav.hh` |

`extern/` is therefore **not** pristine upstream. The game depends on these local changes, so carry them over when updating a library:
- **glow-extras `GlfwApp`** (course framework): a `virtual void onFrameStart()` hook, called right after `updateInput()` in `mainLoop()`, which `Game` overrides to start the per-frame stats. `setCursorMode()` applies `glfwSetInputMode` immediately (upstream: on the next `updateInput()`). An ImGuizmo `SetRect` for `IMGUI_HAS_VIEWPORT` (dead code with this imgui).
- **polymesh OBJ reader** (course framework): `obj_reader::has_valid_texcoords()` / `has_valid_normals()`, used by `Mesh3D.cc`, and a reordered `l` (polyline) loop that adds no edge for a 2-index `l a b` record. Six meshes contain such records (`data/meshes/units/{person,person_artisan,person_soldier,creature}.obj`, `rock1.obj`, `nature/rock1.obj`). With upstream's loop, loading them asserts "vertex is already fully connected".
- **OpenAL `CMakeLists.txt`**: option defaults flipped to OFF for `ALSOFT_DLOPEN`, `ALSOFT_UTILS`, `ALSOFT_EXAMPLES`, `ALSOFT_INSTALL_{HRTF_DATA,AMBDEC_PRESETS,EXAMPLES,UTILS}` and `ALSOFT_UPDATE_BUILD_VERSION`.

## Architecture

### Entry point and main loop

`main.cc` → `Game` (`src/Game.hh/.cc`), which subclasses glow-extras' `glow::glfw::GlfwApp`. `GlfwApp` drives the loop: `update()` runs at a fixed 60 Hz, `render()` once per frame, and `onGui()` builds the ImGui UI (menus, HUD, debug and editor panels). `Game` owns everything: the ECS, every system and factory, the terrain, the camera, and the render settings. Most of the wiring lives in `Game::init()`.

### ECS (`src/ecs/`)

The ECS is based on austinmorlan.com's ECS article. `EngineECS` (`ecs/Engine.hh`) is the facade the rest of the code talks to. It wraps the `ComponentManager`, `SystemManager`, `EventManager`, the `World`, and the spatial hash map (`Hash2D`).

- **Entities are `InstanceHandle`s**, managed by `World` (`advanced/World.hh`). An `Instance` is not a pure ID: it directly holds the render data (VAO, albedo/normal/ARM textures, `transform xform`, bounds, auxiliary models, a simple animation type) plus the component `Signature` bitset and a `destroy` flag. Instances live in a compact pool: destroying one moves the last instance into the freed slot, so `Instance&` references and `Component::instance` pointers are only valid until the next destruction. Keep handles instead.
- Components are stored by value in one fixed-size array per type; removing a component moves the last one into the gap, so don't keep component pointers across removals either.
- Systems and factories hold a `shared_ptr` back to `EngineECS`, so it is never destroyed (reference cycle). `EngineECS::Init` wraps the `World` (owned by `AdvancedFeatures`) in an owning `shared_ptr`, which is only harmless because of that.
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
- **Events** (`ecs/Event.hh`) have a fixed `EventType` enum (collisions, hits, deaths, supply upgrades, ...). `SendFunctionalEvent` dispatches synchronously to listeners registered with `AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::X, Class::Method))`. `SendEvent` only queues events into `GetPendingEvents()` for manual handling; nothing uses that path. All game events are functional: e.g. collision results go from `CollisionSystem` to `GamelogicSystem` and `PhysicsSystem` listeners, deaths and hits to the `ParticleSystem`. `Supply1`, `Supply2` and `MoreWood` are sent but have no listeners. Moving an entity should send `EventType::NewPosition` so `CollisionSystem` updates its collider hash maps (`PhysicsSystem` and the factories do this).

### Systems

- `GamelogicSystem` is by far the largest file (~2.5k lines). It holds all gameplay: resources, building, unit AI and combat, enemy waves, and win/lose state. It also drives some render parameters (desaturation and vignette radius come from game state). It is initialized with references to the audio system and the unit, structure, and item factories.
- `CollisionSystem` handles 2D-only collision (OBB vs. circle shapes on the ground plane) and uses two `Hash2D`s (static and dynamic colliders) for broad-phase. Its static pass also marks destructible static colliders with zero health for destruction.
- `RenderSystem` (`AdvancedRenderSystem.cc`) is a forward renderer. Pass order: point-light shadow cube map → directional shadow map → skybox → scene (`poly` shader) → particles → outlines → bloom → post-processing (tonemap, gamma, sepia, vignette, desaturation, distance fade) → outline resolve. Shaders are loaded from `data/shaders/` by base name: `Program::createFromFile("../data/shaders/poly")` picks up `poly.vsh` + `poly.fsh`. Shader includes use `.glsli`. The `object`/`object_simple` shaders and the test meshes/textures are loaded but unused.
- The other systems are `PhysicsSystem`, `LightSystem`, `ParticleSystem` (CPU-simulated, drawn instanced; `compute_particles.csh` belongs to an unfinished, commented-out GPU variant), `AnimationSystem` (non-skeletal; also flies arrows), `AudioSystem` (OpenAL 3D audio; sound files in `data/audio/`), and `CleanupSystem` (runs last). `EnvironmentSystem` (game clock and day counter, which triggers the daily enemy waves) is not ECS-registered; `Game` creates and updates it directly. `InputSystem` and `AISystem` exist but are not registered (unused).

### Assets and factories

- `GameObjects` (`SharedGameObjects`) is the asset manager. `scanMeshes()` recursively registers every `.obj` under `data/meshes/`, and assets can then be looked up by filename (without extension) or by full path. All models share one palette texture (`data/textures/BigPalette.png`, low-poly flat-color style). `models/` holds the Blender source files, which are not loaded at runtime.
- `Factory` (`src/Factory.hh`) manages named presets: prototype entities that `Create(name, pos, rot, scale)` clones. The subclasses `UnitFactory`, `StructureFactory`, `NatureFactory`, and `ItemFactory` override registration to attach the right components. In `Game::init()`, each factory auto-registers the meshes in its `data/meshes/<folder>/` (`units`, `structures`, `nature`, `items`) with a per-factory scale.
- **World layouts are `.fac` files** in `data/config/`. Each is a binary sequence of `[null-terminated preset name][vec3 translation][quat rotation][size3 scaling]` records. They are loaded at startup (`nature.fac`, `items.fac`, `startStructs.fac`) and by gameplay (e.g. the `supplyroute_*.fac` upgrades). They are written by the in-game editor (toggle with **E**), which spawns presets and saves either all objects or only the newly created ones under a chosen filename. Don't hand-edit these files; regenerate them through the editor.

### Other directories

- `src/advanced/` contains the instance `World`, `transform`, the stats/debug helpers, and the editor. It originated from course framework code.
- `src/utility/` contains the spatial hash map (`Hash2D`), a quadtree (fully commented out), RNG, and WAV loading (`Sound.cc` with dr_wav; only `.wav` files are loaded).
- `src/types/` contains plain data types shared across components (lights, materials, particles, unit and structure properties).
- `docs/` contains early design notes and a ToDo list from the start of the project. They are not authoritative about the current code.
