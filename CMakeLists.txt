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

# >> Dependencies

# The target 'dep_common' is a virtual library.
# It does not contain source files.
#
# Instead, we locate all our project's dependencies,
# and then link them with 'dep_common' so that later,
# other engine components just link with 'dep_common'
# and are transitively linked with our dependencies.
add_library(dep_common INTERFACE)

find_package(glm CONFIG REQUIRED)
target_link_libraries(dep_common INTERFACE glm::glm)

if(CMAKE_SYSTEM_NAME MATCHES Linux)

    find_package(glfw3 CONFIG REQUIRED)
    target_link_libraries(dep_common INTERFACE glfw)

    find_package(imgui CONFIG REQUIRED)
    target_link_libraries(dep_common INTERFACE imgui::imgui)

endif()

# << Dependencies

# >> Components

if(CMAKE_JS_VERSION)
    add_subdirectory(node)
endif()

add_subdirectory(gpu)

add_subdirectory(core)

add_subdirectory(examples)

# << Components

# >> Build system

set(CPACK_PACKAGE_NAME "gengine")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_RPM_PACKAGE_GROUP "gengine")
set(CPACK_PACKAGE_VENDOR "gengine")
include(CPack)

# << Build system