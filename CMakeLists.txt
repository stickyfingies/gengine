# Emscripten compiler settings:
# - https://emscripten.org/docs/tools_reference/settings_reference.html

cmake_minimum_required(VERSION 3.30)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/vcpkg/scripts/buildsystems/vcpkg.cmake")
project(gengine LANGUAGES CXX)

# >> Config

set(CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

if(EMSCRIPTEN)
    # set(CMAKE_EXECUTABLE_SUFFIX ".wasm")
endif()

message(STATUS "CMAKE_SYSTEM is ${CMAKE_SYSTEM}")
message(STATUS "CMAKE_SYSTEM_NAME is ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_TOOLCHAIN_FILE is ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMAKE_PREFIX_PATH is ${CMAKE_PREFIX_PATH}")
message(STATUS "CMAKE_MODULE_PATH is ${CMAKE_MODULE_PATH}")

# if(CMAKE_BUILD_TYPE MATCHES Debug)
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)
# endif()

# << Config

# >> Components

if(CMAKE_JS_VERSION)
    add_subdirectory(source/node)
endif()

add_subdirectory(source/gpu)

add_subdirectory(source/core)

add_subdirectory(source/examples)

# << Components

# >> Build system

set(CPACK_PACKAGE_NAME "gengine")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_RPM_PACKAGE_GROUP "gengine")
set(CPACK_PACKAGE_VENDOR "gengine")
include(CPack)

# << Build system