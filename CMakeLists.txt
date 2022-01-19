cmake_minimum_required(VERSION 3.0)
project(GameDevSS21)

# ==============================================================================
# global settings

# enable folders for visual studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# force C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# ==============================================================================

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Set bin dir
set(BIN_DIR ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${BIN_DIR})


# ==============================================================================
# dependencies

# Add math library
add_subdirectory(extern/typed-geometry)

# Add OpenGL wrapper lib
add_subdirectory(extern/glow)

# Add GLFW lib (with disabled spam)
option(GLFW_BUILD_EXAMPLES "" OFF)
option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_INSTALL "" OFF)
add_subdirectory(extern/glfw)

# Add UI library
add_subdirectory(extern/imgui)

# Add mesh library
add_subdirectory(extern/polymesh)

# Add GLOW Extras lib
add_subdirectory(extern/glow-extras)

# Add foundations lib (containers, basic utility / STL replacement)
add_subdirectory(extern/clean-core)

# Add audio library
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
	OpenAL
)
  set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "Extern")
endforeach()


# ===========================================================================================
# Create target
file(GLOB_RECURSE SOURCES
    "src/*.cc"
    "src/*.hh"
    "data/*.*sh"
    "data/*.glsl*"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
add_executable(${PROJECT_NAME} ${SOURCES})

# Add src/ include dir
target_include_directories(${PROJECT_NAME} PUBLIC src)

# Link libs
target_link_libraries(${PROJECT_NAME} PUBLIC
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
    target_compile_options(${PROJECT_NAME} PUBLIC
        /MP # multi-core compiling
    )
else()
    target_compile_options(${PROJECT_NAME} PUBLIC
        -Wall # useful warnings
        -Wno-unused-variable # not a useful warning
    )

    # required for <filesystem>
    target_link_libraries(${PROJECT_NAME} PUBLIC -lstdc++fs)
endif()
