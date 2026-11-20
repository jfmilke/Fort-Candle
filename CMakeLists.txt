cmake_minimum_required(VERSION 3.21...4.4)
project(GameDevSS21 LANGUAGES C CXX)

# ==============================================================================
# global settings

# enable folders for visual studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# force C++17 (without compiler-specific extensions)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set a default build type for single-config generators (Ninja, Makefiles).
# Multi-config generators (Visual Studio, Ninja Multi-Config) pick the config at build time.
get_property(GAMEDEV_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT GAMEDEV_IS_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# All executables and shared libraries (e.g. the OpenAL DLL) go to bin/<Config>/.
# The game expects to be run with a working directory that has ../data, so run it from bin/.
# Using a generator expression keeps multi-config generators from appending another config subfolder.
set(BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BIN_DIR})


# ==============================================================================
# dependencies

# Most vendored libraries still declare cmake_minimum_required(VERSION 3.0), which
# CMake >= 4.0 refuses. Raise their policy version for the add_subdirectory calls
# below without modifying the third-party code.
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

# Add math library
add_subdirectory(extern/typed-geometry)

# Add OpenGL wrapper lib (both options only affect non-MSVC builds)
# GNU gold was removed from binutils 2.44+, so -fuse-ld=gold fails on current distros
set(GLOW_USE_GOLD_LINKER OFF CACHE BOOL "If true, ld.gold is used for linking")
# -march=native ties the binary to the build machine's CPU; glow falls back to -msse4.2
set(GLOW_ENABLE_MARCH_NATIVE OFF CACHE BOOL "If true, adds -march=native")
add_subdirectory(extern/glow)

# Add GLFW lib (with disabled spam)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/glfw)

# Add UI library
add_subdirectory(extern/imgui)

# Add mesh library
add_subdirectory(extern/polymesh)

# Add GLOW Extras lib
add_subdirectory(extern/glow-extras)

# Add foundations lib (containers, basic utility / STL replacement)
add_subdirectory(extern/clean-core)

# Add audio library (only the library itself, no tools/examples/install rules)
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "" FORCE)
# The JACK backend fails to link on many Linux setups (undefined symbol: jack_error_callback)
set(ALSOFT_BACKEND_JACK OFF CACHE BOOL "Enable JACK backend")
add_subdirectory(extern/openal)

unset(CMAKE_POLICY_VERSION_MINIMUM)

# Folder grouping
foreach(TARGET_NAME
    glfw
    glad
    glow
    glow-extras
    imgui
    polymesh
    typed-geometry
    clean-core
    OpenAL
    alcommon
    ex-common
    update_glfw_config
)
    if(TARGET ${TARGET_NAME})
        set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "Extern")
    endif()
endforeach()


# ===========================================================================================
# Create target
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.cc"
    "src/*.hh"
    "data/*.*sh"
    "data/*.glsl*"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
add_executable(${PROJECT_NAME} ${SOURCES})

# Add src/ include dir
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Link libs
target_link_libraries(${PROJECT_NAME} PRIVATE
    glow
    glow-extras
    polymesh
    imgui
    typed-geometry
    clean-core
    OpenAL
)

# Compile flags

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3     # useful warnings (no longer added by default since CMake policy CMP0092)
        /MP     # multi-core compiling
        /utf-8  # source and execution charset are UTF-8
    )
    # Visual Studio: make the game the startup project and run it from bin/ so that ../data resolves
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall # useful warnings
        -Wno-unused-variable # not a useful warning
    )

    # GCC < 9.1 ships <filesystem> in a separate library
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1)
        target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
    endif()
endif()

if(UNIX AND NOT APPLE)
    # Keep libGL loaded for the whole process (like opengl32.dll on Windows).
    # All GL calls go through glad function pointers, so with the distro default --as-needed
    # libGL never becomes a DT_NEEDED dependency and is only loaded via GLFW's dlopen.
    # glfwTerminate() then unloads it while ~Game() still deletes GL objects -> SIGSEGV on exit.
    find_package(OpenGL REQUIRED)
    target_link_options(${PROJECT_NAME} PRIVATE "LINKER:--no-as-needed")
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
endif()
