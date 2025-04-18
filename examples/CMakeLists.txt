include(cmake/emscripten.cmake)

file(GLOB CPP_FILES "*.cpp")
file(GLOB JS_FILES "*.js")

# For every world.cpp file in examples/
foreach(FILE ${CPP_FILES})

    # Strip the directory prefix and file extension
    get_filename_component(APP_NAME "${FILE}" NAME_WLE)

    message(STATUS "Example ${APP_NAME}")

    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/examples/${APP_NAME}")

    add_executable(${APP_NAME})

    target_link_libraries(${APP_NAME} PRIVATE core gpu)

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
    target_sources(${APP_NAME} PRIVATE ${FILE} "${CMAKE_SOURCE_DIR}/core/main.cpp")

    if(NOT EMSCRIPTEN)
        # Install gengine.exe --> dist/bin/
        install(TARGETS ${APP_NAME} DESTINATION "${CMAKE_INSTALL_BINDIR}")
        # Install data/ --> dist/data/
        install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/" DESTINATION "data/")
    else()
        # Configure Emscripten-specific settings
        configure_emscripten_target(${APP_NAME} ASSETS_PATH "${CMAKE_SOURCE_DIR}/data")
        
        # Install outputs --> dist/examples/APP_NAME/
        install(FILES
            "$<TARGET_FILE_DIR:${APP_NAME}>/index.js"
            "$<TARGET_FILE_DIR:${APP_NAME}>/index.wasm"
            "$<TARGET_FILE_DIR:${APP_NAME}>/index.data"
            "$<TARGET_FILE_DIR:${APP_NAME}>/index.html"
            DESTINATION "examples/${APP_NAME}")
    endif()
endforeach()

# For every JavaScript game script in examples/
foreach(FILE ${JS_FILES})

    # Strip the directory prefix and file extension
    get_filename_component(APP_NAME "${FILE}" NAME_WLE)

    message(STATUS "Example ${APP_NAME}.js")

    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/examples/${APP_NAME}")

    set(HAS_JS TRUE)

    add_executable(${APP_NAME})

    target_link_libraries(${APP_NAME} PRIVATE core gpu)

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
    target_sources(${APP_NAME} PRIVATE ${FILE} "${CMAKE_SOURCE_DIR}/core/main.cpp")

    if(EMSCRIPTEN)
        target_sources(${APP_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/core/ffi/js.web.cpp")
    elseif(NOT EMSCRIPTEN)
        target_sources(${APP_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/core/ffi/js.desktop.cpp")
    endif()

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

        install(FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/${APP_NAME}.js"
            DESTINATION "examples")
    endif()

    if(NOT EMSCRIPTEN)
        # Install gengine.exe --> dist/bin/
        install(TARGETS ${APP_NAME} DESTINATION "${CMAKE_INSTALL_BINDIR}")
        # Install data/ --> dist/data/
        install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/" DESTINATION "data/")
    else()
        # Configure Emscripten-specific settings
        configure_emscripten_target(${APP_NAME} ASSETS_PATH "${CMAKE_SOURCE_DIR}/data")
        
        # Install outputs --> dist/examples/APP_NAME/
        install(FILES
            "$<TARGET_FILE_DIR:${APP_NAME}>/index.js"
            "$<TARGET_FILE_DIR:${APP_NAME}>/index.wasm"
            "$<TARGET_FILE_DIR:${APP_NAME}>/index.data"
            "$<TARGET_FILE_DIR:${APP_NAME}>/index.html"
            DESTINATION "examples/${APP_NAME}")
    endif()
endforeach()
