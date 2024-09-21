cmake_minimum_required(VERSION 3.30)
project(gengine LANGUAGES CXX)

# >> CLI

message(STATUS "Cmake: System name = ${CMAKE_SYSTEM_NAME}")
message(STATUS "Cmake: Toolchain file = ${CMAKE_TOOLCHAIN_FILE}")

# << CLI

# >> Binaries

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

add_executable(${PROJECT_NAME})
add_library(gpu STATIC)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${Vulkan_INCLUDE_DIRS}
)

target_sources(${PROJECT_NAME}
    PRIVATE
    code/main.cpp
    code/assets.cpp
)

if(CMAKE_SYSTEM_NAME MATCHES Linux)
    target_sources(${PROJECT_NAME}
        PRIVATE
        code/physics.cpp
        code/stb/stb_image.cpp
        code/world.wasm.cpp # TODO code/world.native.cpp
    )
elseif(EMSCRIPTEN)
    target_sources(${PROJECT_NAME}
        PRIVATE
        code/world.wasm.cpp
    )

    target_link_options(${PROJECT_NAME} PRIVATE -sALLOW_MEMORY_GROWTH -sUSE_GLFW=3 -sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3 --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/data@data)
    target_link_options(gpu PRIVATE -sALLOW_MEMORY_GROWTH -sUSE_GLFW=3 -sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3)
endif()

if(GPU_BACKEND MATCHES Vulkan)
    target_sources(gpu
        PRIVATE
        code/gpu.vulkan.cpp
    )
elseif(GPU_BACKEND MATCHES GL)
    target_sources(gpu
        PRIVATE
        code/gpu.opengl.cpp
    )
endif()

# << Binaries

# >> Dependencies

find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
target_link_libraries(gpu PRIVATE glm::glm)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)

if(CMAKE_SYSTEM_NAME MATCHES Linux)

    find_package(glfw3 CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

    find_package(glad CONFIG REQUIRED)
    target_link_libraries(gpu PRIVATE glad)
    target_link_libraries(${PROJECT_NAME} PRIVATE glad)

    find_package(Bullet CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE BulletDynamics BulletCollision Bullet3Common LinearMath)
    target_link_directories(${PROJECT_NAME} PRIVATE ${BULLET_LIBRARY_DIRS})

    find_package(Vulkan REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})

    find_package(imgui CONFIG REQUIRED)
    target_link_libraries(gpu PUBLIC imgui::imgui)

endif()

target_link_libraries(${PROJECT_NAME} PRIVATE gpu)

# << Dependencies

# >> Build system

set(CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

set_target_properties(${PROJECT_NAME} gpu
    PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 23
    CMAKE_CXX_STANDARD_REQUIRED ON
)

if(NOT EMSCRIPTEN)
    install(TARGETS ${PROJECT_NAME} DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(DIRECTORY
        "${CMAKE_SOURCE_DIR}/data/"
        DESTINATION "data/")
else()
    install(FILES
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.js"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.wasm"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.data"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.html"
        DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

# Set Package Name
set(CPACK_PACKAGE_NAME "gengine")
# Set The Version Information of The Installation Package
set(CPACK_PACKAGE_VERSION "1.0.0")
# Set The Group Name
set(CPACK_RPM_PACKAGE_GROUP "gengine")
# Set The Vendor Name
set(CPACK_PACKAGE_VENDOR "gengine")
include(CPack)

# << Build system