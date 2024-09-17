cmake_minimum_required(VERSION 3.29)
project(gengine LANGUAGES CXX)

# >> CLI

message(STATUS "Cmake: System name = ${CMAKE_SYSTEM_NAME}")
message(STATUS "Cmake: Toolchain file = ${CMAKE_TOOLCHAIN_FILE}")

# << CLI

# >> Binaries

add_executable(${PROJECT_NAME})
add_library(gpu STATIC)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${Vulkan_INCLUDE_DIRS}
)

target_sources(${PROJECT_NAME}
    PRIVATE
    cpp/main.cpp
)

if(CMAKE_SYSTEM_NAME MATCHES Linux)
    target_sources(${PROJECT_NAME}
        PRIVATE
        cpp/assets.cpp
        cpp/physics.cpp
        cpp/stb/stb_image.cpp
        cpp/world.native.cpp
    )

    target_sources(gpu
        PRIVATE
        cpp/renderer/render-device-vk.cpp
    )
elseif(EMSCRIPTEN)
    target_sources(${PROJECT_NAME}
        PRIVATE
        cpp/world.wasm.cpp
    )

    target_sources(gpu
        PRIVATE
        cpp/renderer/renderer-opengl.cpp
    )

    target_link_options(${PROJECT_NAME} PRIVATE -sUSE_GLFW=3 -sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2)
    target_link_options(gpu PRIVATE -sUSE_GLFW=3 -sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2)
endif()

# << Binaries

# >> Dependencies

find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
target_link_libraries(gpu PRIVATE glm::glm)

if(CMAKE_SYSTEM_NAME MATCHES Linux)

    find_package(glfw3 CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

    find_package(glad CONFIG REQUIRED)
    target_link_libraries(gpu PRIVATE glad)

    find_package(Bullet CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE BulletDynamics BulletCollision Bullet3Common LinearMath)
    target_link_directories(${PROJECT_NAME} PRIVATE ${BULLET_LIBRARY_DIRS})

    find_package(Vulkan REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})

    find_package(assimp CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)

    find_package(imgui CONFIG REQUIRED)
    target_link_libraries(gpu PUBLIC imgui::imgui)

endif()

target_link_libraries(${PROJECT_NAME} PRIVATE gpu)

# << Dependencies

# >> Build system

set(CXX_STANDARD_REQUIRED ON)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

set_target_properties(${PROJECT_NAME} gpu
    PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 23
    CMAKE_CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
)

# << Build system
