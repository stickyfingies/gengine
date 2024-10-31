# For every world.cpp file in examples/
file(GLOB FILES "*.cpp")
foreach(FILE ${FILES})

    # Strip the directory prefix and file extension
    get_filename_component(APP_NAME "${FILE}" NAME_WLE)

    message(STATUS "Example ${APP_NAME}")

    # Determine if this example uses JavaScript or not
    set(HAS_JS FALSE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${APP_NAME}.js")
        set(HAS_JS TRUE)
    endif()

    # Don't mix Duktape with Emscripten
    if(HAS_JS AND EMSCRIPTEN)
        message(STATUS "Skipping ${APP_NAME} for Web because it contains a JS runtime.")
        continue()
    endif()

    # Create an executable for this app
    add_executable(${APP_NAME})

    # Combine engine components
    target_link_libraries(${APP_NAME} PRIVATE dep_common core gpu)

    # Adjust compiler settings
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

    if(HAS_JS)
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

    if(EMSCRIPTEN)
        # This instructs Emscripten how to name [index.data, index.html, index.js, index.wasm]
        set_target_properties(${APP_NAME} PROPERTIES OUTPUT_NAME "index")

        set(EMSCRIPTEN_COMPILER_SETTINGS
            ### Assets
            "--preload-file" "${CMAKE_SOURCE_DIR}/data@data"
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
