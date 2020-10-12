cmake_minimum_required(VERSION 3.12)

include(GNUInstallDirs)

project(gengine LANGUAGES CXX)

# the following line may be commented to reduce iteration speeds but
# is required for clean builds.

# add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/bullet3)

find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
PUBLIC
	${Vulkan_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/vendor/bullet3/src
	${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
PRIVATE
	dl
	${Vulkan_LIBRARIES}
	glfw
	BulletDynamics
	BulletCollision
	LinearMath
	assimp
	${ASSIMP_LIBRARIES}
)

target_sources(${PROJECT_NAME}
PRIVATE
	src/main.cpp
	src/assets.cpp
	src/physics.cpp
	src/glad.cpp
	src/renderer/render-device-vk.cpp
	src/stb/stb_image.cpp
)

set_target_properties(${PROJECT_NAME}
	PROPERTIES
	CXX_STANDARD 17
	CXX_EXTENSIONS OFF
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
)
