cmake_minimum_required(VERSION 3.12)

include(GNUInstallDirs)

project(gengine LANGUAGES CXX)

find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
PRIVATE
	src/main.cpp
	src/physics.cpp
	src/glad.cpp
	src/renderer/renderer.cpp
	src/stb/stb_image.cpp
)

target_include_directories(${PROJECT_NAME}
PUBLIC
	${Vulkan_INCLUDE_DIRS}
	/usr/include/bullet
	${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
PUBLIC
	dl
	${Vulkan_LIBRARIES}
	glfw
	BulletCollision
	BulletDynamics
	LinearMath
	assimp
	${ASSIMP_LIBRARIES}
)

set_target_properties(${PROJECT_NAME}
	PROPERTIES
	CXX_STANDARD 17
	CXX_EXTENSIONS OFF
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
)
