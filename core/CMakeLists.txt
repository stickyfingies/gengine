cmake_minimum_required(VERSION 3.20)

project(core
    VERSION 1.0.0
    DESCRIPTION "Core game engine systems"
    LANGUAGES CXX
)

# Create library target
add_library(core STATIC)
add_library(core::core ALIAS core)

# Configure compiler settings
target_compile_features(core PUBLIC cxx_std_23)
set_target_properties(core
    PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 23
    CMAKE_CXX_STANDARD_REQUIRED ON
)

find_package(gpu REQUIRED)

target_link_libraries(core PRIVATE dep_common gpu::gpu)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(core
    PRIVATE
    # Exclude main.cpp so that 'core' can be used in other libraries
    # main.cpp
    core.cpp
    kernel.cpp
    assets.cpp
    physics.cpp
    scene.cpp
    fps_controller.cpp
    stb/stb_image.cpp
)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(core PRIVATE assimp::assimp)

find_package(Bullet CONFIG REQUIRED)
target_link_libraries(core PRIVATE BulletDynamics BulletCollision Bullet3Common LinearMath)
target_link_directories(core PRIVATE ${BULLET_LIBRARY_DIRS})
