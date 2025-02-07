# The target 'node' is a node.js addon
# https://github.com/cmake-js/cmake-js

cmake_minimum_required(VERSION 3.30)

message(STATUS "CMAKE_JS_INC is ${CMAKE_JS_INC}")

add_definitions(-DNAPI_VERSION=7)
include_directories(${CMAKE_JS_INC})

add_library(node SHARED)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(node PRIVATE glfw)

target_link_libraries(node PRIVATE ${CMAKE_JS_LIB})

target_include_directories(node PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_JS_INC})

set_target_properties(node
    PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 23
    CMAKE_CXX_STANDARD_REQUIRED ON
    PREFIX "" SUFFIX ".node"
)

target_sources(node PRIVATE ${CMAKE_JS_SRC} main.cpp)
