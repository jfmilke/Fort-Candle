# Fort Candle

Result of the GameDevelopment Practical 2021 at RWTH Aachen.
Features a small strategy game which only aims to demonstrate the capabilities of the self-designed game engine.

**Note:** This is a reupload to GitHub from another inaccessible Git repository, now initially reflecting the code state at submission.

## Gameplay

The gameplay mechanics consist of resource management (gold, wood, people), base building, economy and combat.
The goal is to survive against waves of enemies until the victory condition is met.

> You are the commander of the upstream encampment Fort Candle.
> Your goal is to fortify the street from the core cities to allow safe passage into the new frontier sector.
> The defenses are badly needed as danger lurks in the darkness which surrounds these lands.
> A darkness which must not under any circumstances reach the inner camp - keep the lights on!
> Therefore, your duties will encompass not only the construction of said passage but also the concentration of soldiers to defend your position.
> A steady supply of fresh pioneers is brought down the road from the core cities, in case some go.. missing.

## Engine

The engine is tightly coupled to the game itself.
It features the following core elements:

- Entity Component System
    - Supports Events
- Collision System
    - Detects 2D collisions between OBBs and circles (no 3D detection needed)
    - Features a spatial datastructure (Hashmap) to reduce checks
- OpenGL Rendering System
    - Low-Poly Shading
    - Forward Rendering
    - Lighting (Point + Directed + Ambience)
    - Shadows (Point + Directed)
- Particle System
    - Instanced draw calls
- Resource Management
    - Reuse meshes and textures
- Simple UI
- Simple AI (barely..)
- Simple Animations (non-skeletal)
- World saving & loading
- 3D Audio

The rendering equation involved the following processing:

1. Render Depth Cube Map (point lights)
2. Render Depth Map (directed lights)
3. Render Background Cube Map
4. Render Scene (all models)
5. Render Particles
6. Render Post-Processing Effects (Outlines, Bloom, Color Filter, ...)

## Requirements

- CMake 3.8+
- C++17
    - Windows: Visual Studio 2017+
    - Linux: Clang or GCC
- OpenGL 4.5

## Dependencies

All dependencies are included in the `extern` folder.

- GLFW3
- Glad
- GLOW *(modern OpenGL wrapper)*
- ImGUI
- Polymesh *(mesh library)*
- Typed-Geometry *(math library)*
- OpenAL *(audio library)*

## Building

OS-specific build instructions can be seen below.
Be aware that a Debug-Build has *much* slower performance than Release.
RelWithDebugInfo (standard) is a compromise between performance & debugging.

### Windows

- Open CMake-Gui and enter the directory of the source code.
- Now specify where to build the project.
- Click "Generate", choose "Visual Studio 15 2017 Win64" (or newer version that is also Win64) as the generator.
- Open the .sln file in the build folder with Visual Studio.
- Set GameDevSS21 as the Startup Project (right-click on the project -> Set as StartUp Project)
- Build the project
- Run the software from `bin\RelWithDebInfo\GameDevSS21.exe`

If it doesn't work, try setting the working directory to the bin folder (right-click project -> Properties -> Debugging -> Working Directory), `$(TargetDir)/../` should work.

### Linux

- Create a build directory `mkdir build` and change directory `cd build`
- Generate the build files using with CMake via `cmake <path_to_source>`
- Compile the software via `make`
- Run the software via `../bin/<build_mode>/GameDevSS21`

If it doesn't work start CMake with GUI via `cmake-gui <path_to_source>` (while still in the buid directory) and try to toggle off/on possible sources of error.
Specifically, if you get an error from OpenAL you may try to toggle of/on a specific backend.
You can resolve the error `undefined symbol: jack_error_callback` by simply deactivating `ALSOFT_BACKEND_JACK`.
Then click `Configure` and `Generate`, close GUI and again type `make` to compile.

E.g.:
```bash
git clone https://github.com/rovedit/Fort-Candle.git
cd Fort-Candle
mkdir build
cd build
cmake ..
make
../bin/RelWithDebugInfo/GameDevSS21
```

<!-- 
## Building the project (with QtCreator on Linux)

Open `CMakeLists.txt` with `qtcreator` (or `cmake`).

A modern QtCreator will ask you to configure the project.
You can choose the compiler (Clang and GCC work) and which targets to build:

* Debug: no optimizations but with full debug information (you should use this as long as it's fast enough)
* RelWithDebInfo: most optimizations but still a lot of debug information
* Release: full optimization and no debug info (you will have a hard time to debug this)

The hammer icon in the bottom-left corner will build your project (shortcut Ctrl-B).

Before you can run the project you have to setup the Working Directory and the command line arguments.
Go to the `Projects` tab (on the left side) and choose `Run` of your current kit (on the top).
The working directory will be `REPO-DIR/bin/(Debug|Release|RelWithDebInfo)/` (depending on your configuration) but you have to *change* it to `REPO-DIR/bin`.

## Building the project (with Visual Studio 2017 (or newer), on Windows)

Requirements:

* Visual Studio 2017 or newer (C++)
* CMake 3.8+

Steps:
* Open CMake-Gui and enter the directory of the source code.
* Now specify where to build the project (this should *not* be inside the git repository).
* Click "Generate", choose "Visual Studio 15 2017 Win64" (or newer version that is also Win64) as the generator.
* Open the .sln file in the build folder with Visual Studio.
* Set GameDevSS21 as the Startup Project (right-click on the project -> Set as StartUp Project)
* Set working directory to the bin folder (right-click project -> Properties -> Debugging -> Working Directory), `$(TargetDir)/../` should work.
* Build the project
-->
