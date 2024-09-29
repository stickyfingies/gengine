cmake_minimum_required(VERSION 3.30)
project(gengine LANGUAGES CXX)

# >> Config

set(CXX_STANDARD_REQUIRED ON)
include(GNUInstallDirs)
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

message(STATUS "CMAKE_SYSTEM_NAME is ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_TOOLCHAIN_FILE is ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")

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

# >> GPU

# The target 'gpu' is our cross-platform rendering library.
#
# The interface gpu.h is common across implementations,
# but the .cpp implementations depend on what platform we're
# comipling the library for.
add_library(gpu STATIC)

target_link_libraries(gpu PRIVATE dep_common)

set_target_properties(gpu
    PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 23
    CMAKE_CXX_STANDARD_REQUIRED ON
)

if(GPU_BACKEND MATCHES Vulkan)
    # Begin Vulkan implementation
    target_sources(gpu
        PRIVATE
        code/gpu.vulkan.cpp
    )
    find_package(Vulkan REQUIRED)
    target_link_libraries(gpu INTERFACE ${Vulkan_LIBRARIES})
    target_include_directories(gpu PUBLIC ${Vulkan_INCLUDE_DIRS})
    # End Vulkan implementation
elseif(GPU_BACKEND MATCHES GL)
    # Begin OpenGL ES / WebGL implementation
    target_sources(gpu
        PRIVATE
        code/gpu.opengl.cpp
    )

    # Glad is used for OpenGL ES, but is not needed for WebGL.
    if(CMAKE_SYSTEM_NAME MATCHES Linux)
        find_package(glad CONFIG REQUIRED)
        target_link_libraries(dep_common INTERFACE glad)
    endif()
    # End OpenGL ES / WebGL implementation
endif()

# << GPU

# >> Core

add_library(core STATIC)

target_link_libraries(core PRIVATE dep_common)

set_target_properties(core
    PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 23
    CMAKE_CXX_STANDARD_REQUIRED ON
)

target_sources(core
    PRIVATE
    code/main.cpp
    code/assets.cpp
    code/physics.cpp
    code/stb/stb_image.cpp
)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(core PRIVATE assimp::assimp)

find_package(Bullet CONFIG REQUIRED)
target_link_libraries(core PRIVATE BulletDynamics BulletCollision Bullet3Common LinearMath)
target_link_directories(core PRIVATE ${BULLET_LIBRARY_DIRS})

# << Core

# >> Apps

# For every world.cpp file in code/
file(GLOB FILES "code/world.*.cpp")
foreach(FILE ${FILES})

    # Strip the directory prefix and file extension
    get_filename_component(APP_NAME "${FILE}" NAME_WLE)
    message(STATUS "App ${APP_NAME}")

    # Create an executable for this app
    add_executable(${APP_NAME})

    target_link_libraries(${APP_NAME} PRIVATE dep_common core gpu)

    set_target_properties(${APP_NAME}
        PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_STANDARD 23
        CMAKE_CXX_STANDARD_REQUIRED ON
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${APP_NAME}/"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${APP_NAME}/"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${APP_NAME}/"
    )

    target_sources(${APP_NAME} PRIVATE ${FILE})

    if(EMSCRIPTEN)
        set_target_properties(${APP_NAME} PROPERTIES OUTPUT_NAME "index")

        set(EMSCRIPTEN_COMPILER_SETTINGS
            ### Assets
            "--preload-file" "${CMAKE_CURRENT_SOURCE_DIR}/data@data"
            ### Compiler
            # "-sSTRICT"
            # "-sMODULARIZE"
            # "-sEXPORT_ES6"
            "-sFETCH"
            "-sALLOW_MEMORY_GROWTH"
            # "-sNO_DISABLE_EXCEPTION_CATCHING"
            # "-sINCOMING_MODULE_JS_API=preRun,canvas,monitorRunDependencies,print,setStatus"
            ### Graphics
            "-sUSE_GLFW=3"
            "-sUSE_WEBGL2=1"
            "-sMIN_WEBGL_VERSION=2"
            "-sMAX_WEBGL_VERSION=2"
            "-sFULL_ES3"
        )

        target_link_options(${APP_NAME} PRIVATE "${EMSCRIPTEN_COMPILER_SETTINGS}")
    endif()

    if(NOT EMSCRIPTEN)
        # Install gengine.exe --> dist/bin/
        install(TARGETS ${APP_NAME} DESTINATION "${APP_NAME}/${CMAKE_INSTALL_BINDIR}")
        # Install data/ --> dist/data/
        install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/" DESTINATION "${APP_NAME}/data/")
    else()
        # Install outputs --> dist/bin
        install(FILES
            "$<TARGET_FILE_DIR:${APP_NAME}>/${APP_NAME}.js"
            "$<TARGET_FILE_DIR:${APP_NAME}>/${APP_NAME}.wasm"
            "$<TARGET_FILE_DIR:${APP_NAME}>/${APP_NAME}.data"
            "$<TARGET_FILE_DIR:${APP_NAME}>/${APP_NAME}.html"
            DESTINATION "${APP_NAME}/${CMAKE_INSTALL_BINDIR}")
    endif()
endforeach()

# << Apps

# >> Build system

set(CPACK_PACKAGE_NAME "gengine")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_RPM_PACKAGE_GROUP "gengine")
set(CPACK_PACKAGE_VENDOR "gengine")
include(CPack)

# << Build system