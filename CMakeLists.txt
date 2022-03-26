cmake_minimum_required(VERSION 3.12)

include(GNUInstallDirs)
include(./vcpkg/scripts/buildsystems/vcpkg.cmake)

project(gengine LANGUAGES CXX)

add_executable(${PROJECT_NAME})

# Libraries

if (UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl)
endif (UNIX)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)

find_package(Bullet CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE LinearMath Bullet3Common BulletDynamics BulletCollision)

find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})

# Source files

target_include_directories(${PROJECT_NAME}
PUBLIC
	${Vulkan_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/include
)

target_sources(${PROJECT_NAME}
PRIVATE
	src/main.cpp
	src/assets.cpp
	src/physics.cpp
	src/renderer/render-device-vk.cpp
	src/stb/stb_image.cpp
)

# Metadata

set_target_properties(${PROJECT_NAME}
	PROPERTIES
	CXX_STANDARD 17
	CXX_EXTENSIONS OFF
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
)