# Emscripten compiler settings:
# - https://emscripten.org/docs/tools_reference/settings_reference.html

cmake_minimum_required(VERSION 3.30)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/vcpkg/scripts/buildsystems/vcpkg.cmake")
project(gengine LANGUAGES CXX)

# >> Config

set(CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

message(STATUS "CMAKE_SYSTEM is ${CMAKE_SYSTEM}")
message(STATUS "CMAKE_SYSTEM_NAME is ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_TOOLCHAIN_FILE is ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMAKE_PREFIX_PATH is ${CMAKE_PREFIX_PATH}")
message(STATUS "CMAKE_MODULE_PATH is ${CMAKE_MODULE_PATH}")

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

add_subdirectory(typescript)

add_subdirectory(gpu)

add_subdirectory(core)

# << Components

# >> Apps

# For every world.cpp file in examples/
file(GLOB FILES "examples/*.cpp")
foreach(FILE ${FILES})

    # Strip the directory prefix and file extension
    get_filename_component(APP_NAME "${FILE}" NAME_WLE)
    message(STATUS "App ${APP_NAME}")

    # Create an executable for this app
    add_executable(${APP_NAME})

    # Combine engine components
    target_link_libraries(${APP_NAME} PRIVATE dep_common core gpu)

    set_target_properties(${APP_NAME}
        PROPERTIES
        # Standard C++23
        CXX_EXTENSIONS OFF
        CXX_STANDARD 23
        CMAKE_CXX_STANDARD_REQUIRED ON
        # For convenience
        OUTPUT_NAME "${APP_NAME}.bin"
        # Separate world builds
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/${APP_NAME}/"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/${APP_NAME}/"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/${APP_NAME}/"
    )

    # Today, worlds are limited to one source file
    target_sources(${APP_NAME} PRIVATE ${FILE} "core/main.cpp")

    if(NOT EMSCRIPTEN)
        # vcpkg/DukTape
        find_package(
            unofficial-duktape
            CONFIG
            REQUIRED
        )
        target_link_libraries(
            ${APP_NAME}
            PRIVATE
            unofficial::duktape::duktape
        )

        # vcpkg/DukGlue
        find_path(DUKGLUE_INCLUDE_DIRS "dukglue/detail_class_proto.h")
        target_include_directories(
            ${APP_NAME} PRIVATE
            ${DUKGLUE_INCLUDE_DIRS}
        )
    endif()

    if(EMSCRIPTEN)
        # This instructs Emscripten how to name [index.data, index.html, index.js, index.wasm]
        set_target_properties(${APP_NAME} PROPERTIES OUTPUT_NAME "index")

        set(EMSCRIPTEN_COMPILER_SETTINGS
            ### Assets
            "--preload-file" "${CMAKE_CURRENT_SOURCE_DIR}/data@data"
            ### Compiler
            # "-sSTRICT"
            "-sMODULARIZE"
            "-sEXPORT_ES6"
            "-sASSERTIONS"
            "-sDEFAULT_TO_CXX"
            "-sAUTO_JS_LIBRARIES"
            "-sAUTO_NATIVE_LIBRARIES"
            # "-sINCOMING_MODULE_JS_API=preRun,canvas,monitorRunDependencies,print,setStatus"
            "-sALLOW_MEMORY_GROWTH"
            ### Graphics
            "-sUSE_GLFW=3"
            "-sUSE_WEBGL2=1"
            "-sMIN_WEBGL_VERSION=2"
            "-sMAX_WEBGL_VERSION=2"
            "-sFULL_ES3"
        )

        if(CMAKE_BUILD_TYPE MATCHES Debug)
            message(STATUS "Adding Emscripten debug flags")
            # Reference for speeding up linking process:
            # https://github.com/emscripten-core/emscripten/issues/17019
            set(EMSCRIPTEN_COMPILER_SETTINGS
                "${EMSCRIPTEN_COMPILER_SETTINGS}"
                "-sGL_ASSERTIONS=1"
                "-sERROR_ON_WASM_CHANGES_AFTER_LINK"
                "-sWASM_BIGINT"
                "-O1"
            )
        elseif(CMAKE_BUILD_TYPE MATCHES Release)
            message(STATUS "Adding Emscripten debug flags")
            set(EMSCRIPTEN_COMPILER_SETTINGS
                "${EMSCRIPTEN_COMPILER_SETTINGS}"
                "-sASSERTIONS=0"
                "-O3"
            )
        endif()

        target_link_options(${APP_NAME} PRIVATE "${EMSCRIPTEN_COMPILER_SETTINGS}")
    endif()

    if(NOT EMSCRIPTEN)
        # Install gengine.exe --> dist/bin/
        install(TARGETS ${APP_NAME} DESTINATION "${CMAKE_INSTALL_BINDIR}")
        # Install data/ --> dist/data/
        install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/" DESTINATION "data/")
    else()
        # Install outputs --> dist/bin
        install(FILES
            "$<TARGET_FILE_DIR:${APP_NAME}>/index.js"
            "$<TARGET_FILE_DIR:${APP_NAME}>/index.wasm"
            "$<TARGET_FILE_DIR:${APP_NAME}>/index.data"
            "$<TARGET_FILE_DIR:${APP_NAME}>/index.html"
            DESTINATION "examples/${APP_NAME}")
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