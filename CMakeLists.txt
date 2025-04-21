# Reference: https://emscripten.org/docs/tools_reference/settings_reference.html

cmake_minimum_required(VERSION 3.30)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/vcpkg/scripts/buildsystems/vcpkg.cmake")
project(gengine 
    VERSION 1.0.0
    DESCRIPTION "Cross-platform game engine with multiple rendering backends"
    LANGUAGES CXX
)

# Make version info available to all subprojects
set(GENGINE_VERSION ${PROJECT_VERSION})

#### Configuration

set(CXX_STANDARD_REQUIRED ON)

# Print out some useful information
message(STATUS "CMAKE_GENERATOR is ${CMAKE_GENERATOR}")
message(STATUS "CMAKE_SYSTEM is ${CMAKE_SYSTEM}")
message(STATUS "CMAKE_SYSTEM_NAME is ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_TOOLCHAIN_FILE is ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMAKE_PREFIX_PATH is ${CMAKE_PREFIX_PATH}")
message(STATUS "CMAKE_MODULE_PATH is ${CMAKE_MODULE_PATH}")

# Set default GPU backend if not specified
if(NOT DEFINED GPU_BACKEND)
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
        set(GPU_BACKEND "GL" CACHE STRING "GPU backend to use (GL or Vulkan)")
    elseif(EMSCRIPTEN)
        set(GPU_BACKEND "GL" CACHE STRING "GPU backend to use (GL or Vulkan)")
    else()
        set(GPU_BACKEND "Vulkan" CACHE STRING "GPU backend to use (GL or Vulkan)")
    endif()
endif()
message(STATUS "Using GPU_BACKEND: ${GPU_BACKEND}")

include(GNUInstallDirs)

add_custom_target(graphviz ALL # Generate a diagram of the build process (build.png)
    COMMAND ${CMAKE_COMMAND} "--graphviz=graphviz/build.dot" .
    COMMAND dot -Tpng graphviz/build.dot -o build.png
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

#### Components

if(CMAKE_JS_VERSION)
    add_subdirectory(node)
endif()

add_subdirectory(gpu)

add_subdirectory(core)

add_subdirectory(examples)

#### Packaging

set(CPACK_PACKAGE_NAME "gengine")
set(CPACK_PACKAGE_VERSION ${GENGINE_VERSION})
set(CPACK_RPM_PACKAGE_GROUP "gengine")
set(CPACK_PACKAGE_VENDOR "gengine")
include(CPack)