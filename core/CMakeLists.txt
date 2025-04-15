cmake_minimum_required(VERSION 3.30)

# The target 'core' is the main game engine, which combines
# subsystems together into a cohesive package with an API
# for building game worlds.
project(core
    VERSION ${GENGINE_VERSION}
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

# Use internal gpu target directly instead of find_package
# find_package(gpu CONFIG REQUIRED) # This would be used for external projects
target_link_libraries(core PRIVATE gpu)

# Required dependency for all platforms
find_package(glm CONFIG REQUIRED)
target_link_libraries(core PUBLIC glm::glm)

# Required dependency for all platforms
find_package(assimp CONFIG REQUIRED)
target_link_libraries(core PRIVATE assimp::assimp)

# Required dependency for all platforms
find_package(Bullet CONFIG REQUIRED)
target_link_libraries(core PRIVATE BulletDynamics BulletCollision Bullet3Common LinearMath)
target_link_directories(core PRIVATE ${BULLET_LIBRARY_DIRS})

# Platform-specific dependencies
if(CMAKE_SYSTEM_NAME MATCHES Linux)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(imgui CONFIG REQUIRED)
    target_link_libraries(core PUBLIC glfw imgui::imgui)
endif()

# Configure include directories with proper interfaces
target_include_directories(core 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

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

# Install header files
install(
    FILES 
        core.h
        kernel.h
        assets.h
        physics.h
        scene.h
        fps_controller.h
        camera.hpp
        common.h
        window.h
        world.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install target
install(TARGETS core
    EXPORT core-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Export target
export(TARGETS core
    FILE ${CMAKE_CURRENT_BINARY_DIR}/core-targets.cmake
)

# Generate the cmake version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/core-version.cmake"
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Configure the cmake config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/core-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/core-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/core" # Corrected install path
)

# Install the version and config file
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/core-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/core-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/core" # Corrected install path
)