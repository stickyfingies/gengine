cmake_minimum_required(VERSION 3.12)

project(gengine LANGUAGES CXX)

find_package(Vulkan REQUIRED)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/bullet3-2.88)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/glfw-3.3)

set(BUILD_SHARED_LIBS ON)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/assimp-5.0.0)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
PRIVATE
	src/main.cpp
	src/physics.cpp
	src/glad.cpp
	src/renderer/renderer.cpp
)

target_include_directories(${PROJECT_NAME}
PUBLIC
	${Vulkan_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/external/bullet3-2.88/src
	${CMAKE_SOURCE_DIR}/external/glfw-3.3/include
	${CMAKE_SOURCE_DIR}/external/assimp-5.0.0/include
)

target_link_libraries(${PROJECT_NAME}
PUBLIC
	${Vulkan_LIBRARIES}
	glfw
	glu32
	BulletCollision
	BulletDynamics
	LinearMath
	assimp
	${ASSIMP_LIBRARIES}
)

target_compile_options(${PROJECT_NAME}
PRIVATE
	/std:c++latest
	/MTd
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/bin"
)