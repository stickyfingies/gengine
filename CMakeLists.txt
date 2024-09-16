cmake_minimum_required(VERSION 3.29)

project(gengine LANGUAGES CXX)

include(GNUInstallDirs)
include(./.vcpkg/scripts/buildsystems/vcpkg.cmake)

add_executable(${PROJECT_NAME})

add_library(gpu STATIC)

# Dependencies

# Libraries

if (UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl)
endif (UNIX)

find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
target_link_libraries(gpu PRIVATE glm::glm)

if (NOT EMSCRIPTEN)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

find_package(Bullet CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE BulletDynamics BulletCollision Bullet3Common LinearMath)
target_link_directories(${PROJECT_NAME} PRIVATE ${BULLET_LIBRARY_DIRS})

find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})

find_package(assimp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(gpu PUBLIC imgui::imgui)

target_link_libraries(${PROJECT_NAME} PRIVATE gpu)

endif (NOT EMSCRIPTEN)

# Source files

target_include_directories(${PROJECT_NAME}
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_sources(${PROJECT_NAME}
PRIVATE
	src/main.cpp
	src/assets.cpp
	src/physics.cpp
	src/world.cpp
	src/stb/stb_image.cpp
)

if (EMSCRIPTEN)
    target_link_options(${PROJECT_NAME} PRIVATE -sUSE_GLFW=3)
endif (EMSCRIPTEN)

target_sources(gpu
PRIVATE
	src/renderer/render-device-vk.cpp
	# src/renderer/renderer-opengl.cpp
)

# Metadata

set(CXX_STANDARD_REQUIRED ON)

set_target_properties(${PROJECT_NAME} gpu
PROPERTIES
	CXX_EXTENSIONS OFF
	CXX_STANDARD 23
	CMAKE_CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
)