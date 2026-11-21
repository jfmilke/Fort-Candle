cmake_minimum_required(VERSION 3.21...4.4)
project(GameDevSS21 LANGUAGES C CXX)

# ==============================================================================
# global settings

# enable folders for visual studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# force C++23 (without compiler-specific extensions)
# glow and polymesh read CMAKE_CXX_STANDARD, so it is set globally; OpenAL pins its own targets to C++17.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CMake < 4.5 maps C++23 to /std:c++latest on MSVC, which is the C++26 working draft.
# MSVC 19.52 (Build Tools 14.52) has /std:c++23, which CMake >= 4.5 uses natively (cl >= 19.52.37029).
# The Visual Studio generator of CMake < 4.5 only knows the older /std:c++23preview spelling:
# it turns it into LanguageStandard=stdcpp23, which MSBuild passes as /std:c++23.
# TODO: remove this block once cmake_minimum_required is at least 4.5.
if(CMAKE_VERSION VERSION_LESS 4.5 AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.52)
    if(CMAKE_GENERATOR MATCHES "^Visual Studio")
        set(CMAKE_CXX23_STANDARD_COMPILE_OPTION "-std:c++23preview")
    else()
        set(CMAKE_CXX23_STANDARD_COMPILE_OPTION "-std:c++23")
    endif()
    set(CMAKE_CXX23_EXTENSION_COMPILE_OPTION ${CMAKE_CXX23_STANDARD_COMPILE_OPTION})
endif()

# No C++20 modules are used; without this, CMake (policy CMP0155) scans every game source for imports
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

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

# Add foundations lib (containers, basic utility / STL replacement).
# It must come first: typed-geometry, polymesh, glow and glow-extras require the clean-core target,
# and typed-geometry and polymesh are set up with its CMake helpers (arcana_add_library).
# Those helpers link every arcana library with ld.mold on Linux unless this is off.
set(CC_LINKER_MOLD OFF CACHE BOOL "If true, uses the mold linker (default, must be installed)")
add_subdirectory(extern/clean-core)

# Add math library
add_subdirectory(extern/typed-geometry)

# Add OpenGL wrapper lib (both options only affect non-MSVC builds)
# glow links with ld.mold on Linux unless this is off
set(GLOW_USE_MOLD_LINKER OFF CACHE BOOL "If true, ld.mold is used for linking")
# -march=native ties the binary to the build machine's CPU; glow falls back to -msse4.2
set(GLOW_ENABLE_MARCH_NATIVE OFF CACHE BOOL "If true, adds -march=native")
add_subdirectory(extern/glow)

# Add GLFW lib (with disabled spam)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
# Linux: build for X11 only (Wayland sessions run the game through XWayland)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "Build support for Wayland")
# GLFW >= 3.4 refuses to configure while the GLFW_USE_WAYLAND cache entry of older GLFW versions exists
unset(GLFW_USE_WAYLAND CACHE)
add_subdirectory(extern/glfw)

# Add UI library (Dear ImGui + ImGuizmo; the game does not use ImPlot)
set(IMGUI_USE_IMPLOT OFF CACHE BOOL "add implot")
add_subdirectory(extern/imgui)

# Add mesh library
add_subdirectory(extern/polymesh)

# Add GLOW Extras lib
add_subdirectory(extern/glow-extras)

# Add audio library (only the library itself, no tools/examples/install rules)
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_HRTF_DATA OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_AMBDEC_PRESETS OFF CACHE BOOL "" FORCE)
# The game does not need JACK. With OpenAL 1.21.1 and ALSOFT_DLOPEN=OFF it failed to link
# (undefined symbol: jack_error_callback); not tested with the current version.
set(ALSOFT_BACKEND_JACK OFF CACHE BOOL "Enable JACK backend")
add_subdirectory(extern/openal)

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
    alsoft.common
    alsoft.excommon
    alsoft.fmt
    clang-tidy-check
    update_mappings
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
