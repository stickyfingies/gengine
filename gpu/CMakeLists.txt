cmake_minimum_required(VERSION 3.30)

# The target 'gpu' is our cross-platform rendering library.
# The interface gpu.h is common across implementations, but
# the .cpp implementations depend on what platform we're
# compiling the library for.
project(gpu
    VERSION ${GENGINE_VERSION}
    DESCRIPTION "Cross-platform GPU library"
    LANGUAGES CXX
)

include(GNUInstallDirs)

message(STATUS "${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "${VCPKG_TARGET_TRIPLET}")
message(STATUS "${CMAKE_PREFIX_PATH}")
message(STATUS "$ENV{VCPKG_ROOT}")

# Create library target
add_library(gpu STATIC)
add_library(gpu::gpu ALIAS gpu)

# Configure compiler settings
target_compile_features(gpu PUBLIC cxx_std_23)
set_target_properties(gpu
    PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 23
    CMAKE_CXX_STANDARD_REQUIRED ON
)

# Where to find C++ headers and includes
target_include_directories(gpu PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Supported GPU backends
set(SUPPORTED_BACKENDS "Vulkan" "GL")

# Verify GPU backend configuration
if(NOT DEFINED GPU_BACKEND OR NOT GPU_BACKEND IN_LIST SUPPORTED_BACKENDS)
    message(FATAL_ERROR "GPU_BACKEND must be one of: ${SUPPORTED_BACKENDS}")
endif()

# Backend-specific compilation options
target_compile_definitions(gpu PRIVATE
    GPU_BACKEND="${GPU_BACKEND}"
)
if(GPU_BACKEND MATCHES "Vulkan")
    target_sources(gpu PRIVATE src/gpu.vulkan.cpp)
    find_package(Vulkan REQUIRED)
    target_link_libraries(gpu PRIVATE Vulkan::Vulkan)
elseif(GPU_BACKEND MATCHES "GL")
    target_sources(gpu PRIVATE src/gpu.opengl.cpp)

    # Glad is used for OpenGL ES, but is not needed for WebGL.
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
        find_package(glad CONFIG REQUIRED)
        target_link_libraries(gpu PRIVATE glad::glad)
    endif()
endif()

# Required dependencies for all platforms
find_package(glm CONFIG REQUIRED)
target_link_libraries(gpu PUBLIC glm::glm)

# Platform-specific dependencies
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(glfw3 CONFIG REQUIRED)
    find_package(imgui CONFIG REQUIRED)
    target_link_libraries(gpu PUBLIC glfw imgui::imgui)
endif()

# Shader support and such
if(NOT EMSCRIPTEN)
    find_package(glfw3 REQUIRED)
    find_package(Lua REQUIRED)
    find_package(sol2 REQUIRED)
    find_package(spirv_cross_core REQUIRED)
    find_package(spirv_cross_glsl REQUIRED)
    find_package(spirv_cross_cpp REQUIRED)
    find_package(unofficial-shaderc REQUIRED)

    # This affects gpu
    target_sources(gpu PRIVATE src/shaders.cpp)
    target_link_libraries(gpu PRIVATE spirv-cross-core
        spirv-cross-glsl
        spirv-cross-cpp
        unofficial::shaderc::shaderc)

    # This creates gpu-lua
    add_executable(gpu-lua)
    target_sources(gpu-lua PRIVATE src/gpu-lua.cpp)
    target_include_directories(gpu-lua PRIVATE ${LUA_INCLUDE_DIR})
    target_link_libraries(gpu-lua PRIVATE
        ${LUA_LIBRARIES}
        gpu
        sol2::sol2
        glfw
    )
endif()

# Install target
install(TARGETS gpu
    EXPORT gpu-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Export target
export(TARGETS gpu
    FILE ${CMAKE_CURRENT_BINARY_DIR}/gpu-targets.cmake
)

# Generate the cmake version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/gpu-version.cmake"
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Configure the cmake config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/gpu-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/gpu-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/gpu" # Corrected install path
)

# Install the version and config file
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/gpu-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/gpu-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/gpu" # Corrected install path
)
