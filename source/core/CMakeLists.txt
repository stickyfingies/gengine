cmake_minimum_required(VERSION 3.20)

# The target 'core' is the main game engine, which combines
# subsystems together into a cohesive package with an API
# for building game worlds.
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

# Required dependency for all platforms
# find_package(gpu REQUIRED)
target_link_libraries(core PRIVATE gpu::gpu)

# Required dependency for all platforms
find_package(glm CONFIG REQUIRED)
target_link_libraries(core INTERFACE glm::glm)

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
    target_link_libraries(core INTERFACE glfw imgui::imgui)
endif()

# TODO - change this to use build/install interfaces
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

# Install header file
install(FILES core.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

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