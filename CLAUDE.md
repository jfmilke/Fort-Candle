# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Fort Candle is a small low-poly strategy game (RWTH Aachen GameDev practical 2021) built on a custom engine that is tightly coupled to the game. C++23, OpenGL 4.5, CMake. The CMake project/executable name is `GameDevSS21`. All dependencies are vendored in `extern/` (glfw, glow, glow-extras, imgui, polymesh, typed-geometry, clean-core, openal, dr-lib); treat them as third-party. Prefer workarounds in the top-level `CMakeLists.txt` or the game; if a library must change, do it in a separate commit per library named `extern/<lib>: ...` (see [Vendored libraries](#vendored-libraries-extern)).

There are no tests, linters, or CI. "Verifying" a change means building and, where relevant, running the game.

## Build & run

```powershell
cmake -S . -B build
cmake --build build --config RelWithDebInfo --parallel
```

- Requires CMake ≥ 3.21 and is tested with CMake 4.4 and 4.5 and MSVC 19.52 (Visual Studio 2026). Every `extern/` CMake project declares at least 3.10 or a version range reaching past it (GLFW `3.16...3.28`, OpenAL's bundled fmt `3.8...3.28`), so CMake 4 configures them without warnings or a policy workaround.
- Single-config generators (Ninja, Makefiles) default to `CMAKE_BUILD_TYPE=RelWithDebInfo`. Multi-config generators (Visual Studio) pick the config at build time, and a plain `cmake --build build` builds **Debug**. Debug builds are *much* slower at runtime.
- The executable and `OpenAL32.dll` are written to `bin/<Config>/` (e.g. `bin/RelWithDebInfo/GameDevSS21.exe`). In Visual Studio, `GameDevSS21` is the startup project and its debugger working directory is already `bin/`.
- `bin/`, `build/`, and `build_vs/` are gitignored.
- The game target compiles with `/W3 /MP /utf-8` on MSVC (keep sources UTF-8) and `-Wall -Wno-unused-variable` elsewhere.
- C++23 is set globally (`CMAKE_CXX_STANDARD`, which glow and polymesh read), so the extern libraries build as C++23 too, except OpenAL, which pins its targets to C++17. On MSVC, CMake ≥ 4.5 passes `/std:c++23` (cl ≥ 19.52.37029). CMake < 4.5 passes `/std:c++latest`, i.e. the C++26 draft, so for MSVC ≥ 19.52 the top-level `CMakeLists.txt` overrides it with `/std:c++23` there (the Visual Studio generator gets `/std:c++23preview`, which it turns into `LanguageStandard=stdcpp23`, which MSBuild passes as `/std:c++23`). Remove that block once the minimum is CMake 4.5. C++20 module scanning is off (`CMAKE_CXX_SCAN_FOR_MODULES OFF`); nothing uses modules.
- Some extern flags are PUBLIC and therefore also apply to the game: clean-core adds `/fp:precise` and `/we4715 /we4477 /we4474` (missing return and printf format warnings are errors) on MSVC and `-ffp-contract=off` (no FMA contraction) elsewhere; glow adds `-msse4.2` on GCC/Clang.
- Linux (checked on Ubuntu 26.04 in WSL2 with GCC 15.2 / Clang 21.1 / CMake 4.2):
  - Packages: `libgl-dev`, `libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev`, and `libpulse-dev` and/or `libasound2-dev` (without one of them OpenAL has no audio backend). GLFW is built X11-only (`GLFW_BUILD_WAYLAND=OFF`); Wayland sessions use XWayland.
  - The top-level `CMakeLists.txt` sets `CC_LINKER_MOLD=OFF` and `GLOW_USE_MOLD_LINKER=OFF`: clean-core (which also configures typed-geometry and polymesh) and glow link with `-fuse-ld=mold` by default. It also sets `GLOW_ENABLE_MARCH_NATIVE=OFF` (portable binaries).
  - The game links `OpenGL::GL` with `--no-as-needed`. Without that, libGL is only loaded by GLFW's `dlopen`, `glfwTerminate()` unloads it, and `~Game()` then crashes (SIGSEGV) while deleting GL objects. glow still links the empty `${OpenGL_LIBRARIES}`, and glow-extras' `GlfwApp` still terminates GLFW before the app object is destroyed.
  - `glow`, `glow-extras` and `glad` are shared libraries on Linux; they land next to the executable in `bin/<Config>/` and are found via RPATH.
  - The OpenAL JACK backend is off (`ALSOFT_BACKEND_JACK=OFF`); the game does not need it. With OpenAL 1.21.1 it failed to link (`undefined symbol: jack_error_callback`) because the vendored copy defaulted to `ALSOFT_DLOPEN=OFF`. The current version uses the upstream default (ON), which should fix that, but JACK was not tested.
  - The 2021 `extern/` snapshots (replaced in 2026, see below) needed six small fixes for GCC ≥ 14 and binutils ≥ 2.44. The current versions compile unpatched with GCC 15, Clang 21 and MSVC 19.52.

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

Each library in `extern/` is an unmodified upstream tree: the committed files are byte-identical to upstream, line endings included. Local patches sit on top as separate `extern/<lib>: ...` commits. The arcana/Glow libraries have no maintained release tags (`develop` is their only active branch), so pin commits.

| Directory | Upstream | Version | Local differences |
|---|---|---|---|
| `clean-core` | https://github.com/project-arcana/clean-core | develop `08aa8fc` (2026-06-20) | `tests/` left out |
| `typed-geometry` | https://github.com/project-arcana/typed-geometry | develop `6eaf455` (2026-04-17) | `tests/` left out |
| `polymesh` | https://github.com/project-arcana/polymesh | develop `84b9a5d` (2026-05-28) | **OBJ reader patch** `35066b0` (below) |
| `glow` (bundles `extern/glad`) | https://gitlab.vci.rwth-aachen.de:9000/Glow/glow | develop `552a908` (2026-05-28) | none |
| `glow-extras` | https://gitlab.vci.rwth-aachen.de:9000/Glow/glow-extras | develop `20623d7` (2026-06-24) | **`GlfwApp::onFrameStart()` hook** `5d26a23` (below) |
| `imgui` | `imgui-lean` wrapper fork: https://gitlab.vci.rwth-aachen.de:9000/ptrettner/imgui-lean | master `f37145b` (2026-01-25) | none. Bundles imgui 1.91.2 WIP (ocornut `67cd4ead6`) with its GLFW/OpenGL3 backends, ImGuizmo 1.92.5 WIP and ImPlot 0.17 (not built: `IMGUI_USE_IMPLOT=OFF`) |
| `glfw` | https://github.com/glfw/glfw | tag `3.5.1` (`d9d6f0f1`, 2026-07-31) | none |
| `openal` | https://github.com/kcat/openal-soft | tag `1.24.3` (`dc7d7054a`, 2025-03-28) | none |
| `dr-lib` | https://github.com/mackron/dr_libs | dr_wav 0.12.19, dr_mp3 0.6.27, dr_flac 0.12.29 | not used by the build; the game compiles its own identical copy `src/utility/dr_wav.hh` |

History: `1d74651` (2022-01-19) imported 2021 snapshots as plain files, with a few course-framework patches. They were replaced by the versions above in `b2c9cee` (OpenAL), `87f741e` (arcana/Glow set and imgui) and `e745ea4` (GLFW). dr-lib is still from the original import.

The game depends on these local patches, so carry them over when updating a library:
- **glow-extras `GlfwApp`** (`5d26a23`, from the course framework): a `virtual void onFrameStart()` hook, called right after `updateInput()` in `mainLoop()`, which `Game` overrides to start the per-frame stats. The course framework also made `setCursorMode()` take effect immediately; that change was dropped, so a new cursor mode applies in the next `updateInput()`.
- **polymesh OBJ reader** (`35066b0`, from the course framework): `obj_reader::has_valid_texcoords()` / `has_valid_normals()`, used by `Mesh3D.cc`, and a reordered `l` (polyline) loop that adds no edge for a 2-index `l a b` record. Six meshes contain such records (`data/meshes/units/{person,person_artisan,person_soldier,creature}.obj`, `rock1.obj`, `nature/rock1.obj`). With upstream's loop, loading them asserts "vertex is already fully connected".

Updating `extern/`:
- clean-core, typed-geometry, polymesh, glow and glow-extras move together: the others stop at configure time without the `clean-core` target, so `add_subdirectory(extern/clean-core)` comes first. imgui-lean moves with glow-extras (glow-extras adapts to each imgui-lean update; older glow-extras does not compile against newer imgui).
- Import upstream trees as git objects (copy the objects of `<rev>^{tree}` into this repository, `git read-tree --prefix=extern/<lib>/ <tree>`, `git checkout -- extern/<lib>`), not by copying a checkout. With `core.autocrlf=true`, copied files get their line endings changed.
- Known blockers (2026-11): OpenAL 1.25.2 fails on MSVC 19.52 (C3889 in `alc.cpp`) and Clang 21 (`-Wfunction-effects`). imgui ≥ 1.92.7 removed the `Combo`/`ListBox` getter overload that glow-extras' `debugging/imgui-util.cc` still uses.
- typed-geometry's `tg::perspective_opengl` takes a `tg::horizontal_fov_t` since 2024 (it used to treat the angle as vertical FOV). The game converts its 60° vertical FOV in `RenderSystem::UpdateCamera` and in the editor gizmo (`Game::onGui`).
- The game still uses deprecated APIs that newer versions may remove: `tg::distance_sqr(vec, vec)` and `tg::length_sqr(pos)` (deprecation warnings), and the old imgui names `ImGuiCol_TabActive`/`TabUnfocused`/`TabUnfocusedActive` (renamed in 1.90.9, available while `IMGUI_DISABLE_OBSOLETE_FUNCTIONS` is not defined).
- `ImGuiCol_NavHighlight` and single-argument `ImGui::PushFont` are still current in the vendored imgui 1.91.2, but change in newer versions: 1.91.4 renames `NavHighlight` to `NavCursor` (the old name becomes obsolete), and 1.92 adds a font size parameter to `PushFont`.

## Architecture

### Entry point and main loop

`main.cc` → `Game` (`src/Game.hh/.cc`), which subclasses glow-extras' `glow::glfw::GlfwApp`. `GlfwApp` drives the loop: `update()` runs at a fixed 60 Hz, `render()` once per frame, and `onGui()` builds the ImGui UI (menus, HUD, debug and editor panels). `Game` owns everything: the ECS, every system and factory, the terrain, the camera, and the render settings. Most of the wiring lives in `Game::init()`.

### ECS (`src/ecs/`)

The ECS is based on austinmorlan.com's ECS article. `EngineECS` (`ecs/Engine.hh`) is the facade the rest of the code talks to. It wraps the `ComponentManager`, `SystemManager`, `EventManager`, the `World`, and the spatial hash map (`Hash2D`).

- **Entities are `InstanceHandle`s**, managed by `World` (`advanced/World.hh`). An `Instance` is not a pure ID: it directly holds the render data (VAO, albedo/normal/ARM textures, `transform xform`, bounds, auxiliary models, a simple animation type) plus the component `Signature` bitset and a `destroy` flag. Instances live in a compact pool: destroying one moves the last instance into the freed slot, so `Instance&` references are only valid until the next destruction. Keep handles instead. A handle encodes the instance's pool slot and a generation; `World::SlotIndex` maps a live handle to its stable slot (unlike the packed index, it never changes).
- Components are stored by value in one fixed-size sparse set per type (`ComponentArray`): a dense component array plus a `World` slot -> index table, so lookups are two array reads and never allocate. Removing a component moves the last one into the gap, so don't keep component pointers across removals either. Lookups need the owner alive in the `World`, so `EngineECS::DestroyInstance` detaches systems, spatial hash and components before the `World` frees the slot.
- **Ownership:** `Game` owns `EngineECS` (`std::unique_ptr`), which owns the managers and, through the `SystemManager`, the systems. Systems, factories, `Terrain` and `AdvancedFeatures` keep a non-owning `EngineECS*` (`Init(EngineECS&)`). `Game::onClose()` destroys instances, factories, systems and the ECS while the GL context still exists; `GlfwApp` terminates GLFW right after it.
- **Limits** are in `Constants.hh`: `MAX_COMPONENTS = 32`, `MAX_INSTANCES = 2048`.
- **Components** (`src/components/`, all included through `Components.hh`) derive from `Component`, which stores the owner's `handle_value`. They must satisfy the `EcsComponent` concept (default-constructible, copyable). `Component` is not polymorphic; operations that must run on every component of an entity without naming its type belong on `IComponentArray` (like `TryRemoveData`, `CloneEntityComponent`). Several component structs can share one header, e.g. `UnitComp.hh` defines `Living`, `Mortal`, `Attacker`, etc. Each component type gets a process-wide `ComponentType` ID on first use (`ComponentManager::TypeId`), which indexes the component arrays directly; systems are looked up by `std::type_index`.
- **Systems** (`src/systems/`) derive from `System` and receive every entity whose signature contains the system's signature, collected in `mEntities` (a `std::set<InstanceHandle>`). Systems can override `AddEntity`/`RemoveEntity`/`RemoveAllEntities` (mark them `override`) to keep their own containers. Systems create and change entities while iterating `mEntities`, which `std::set` tolerates (a vector would not); never destroy the entity at the current iterator, use `MarkDestruction`.

Adding a new component or system requires matching edits in `Game.cc`/`Game.hh`:
1. `mECS->RegisterComponent<T>()` in `Game::init()`. Using a component before it is registered asserts.
2. `mECS->RegisterSystem<S, Required...>()`: the system receives every entity that has all `Required` components (none: every entity). Register systems before creating entities; existing entities are not re-checked.
3. Call the system's `Init(*mECS, ...)`, call its `Update(dt)` from `Game::update()`, and reset its `Game` pointer in `Game::onClose()`. `Update` returns elapsed microseconds, which feed the debug timing UI.

Notes:
- **`Prototype` entities are never added to systems** (`EngineECS::UpdateSystems`). Factory presets carry this component, so they exist only as templates to clone.
- **Destruction is deferred.** Call `mECS->MarkDestruction(handle)`, and `CleanupSystem` destroys marked instances at the end of the update. That removes them from all systems, the spatial hash and all component arrays. The ECS asserts (`CC_ASSERT_MSG`) fire in Debug and RelWithDebInfo, e.g. on `GetComponent` for a missing component.
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
- `src/utility/` contains the spatial hash map (`Hash2D`; `FindNear` returns a sorted, deduplicated `std::vector`), a quadtree (fully commented out), RNG, and WAV loading (`Sound.cc` with dr_wav; only `.wav` files are loaded).
- `src/types/` contains plain data types shared across components (lights, materials, particles, unit and structure properties).
- `docs/` contains early design notes and a ToDo list from the start of the project. They are not authoritative about the current code.
