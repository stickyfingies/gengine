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
        "-sEXPORTED_FUNCTIONS=_main,_kernel_create,_kernel_running,_kernel_update,_kernel_destroy,_matrix_create,_matrix_destroy,_matrix_translate,_matrix_scale,_scene_create,_scene_destroy,_scene_load_model,_scene_create_capsule,_scene_create_sphere"
        "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap"
        "-sINVOKE_RUN=0"
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
        message(STATUS "Adding Emscripten release flags")
        set(EMSCRIPTEN_COMPILER_SETTINGS
            "${EMSCRIPTEN_COMPILER_SETTINGS}"
            "-sASSERTIONS=0"
            "-O3"
        )
    endif()

    target_link_options(${APP_NAME} PRIVATE "${EMSCRIPTEN_COMPILER_SETTINGS}")

    target_link_libraries(${APP_NAME} PRIVATE embind)

    configure_file("${CMAKE_SOURCE_DIR}/web/index.html" "${CMAKE_BINARY_DIR}/examples/${APP_NAME}/index.html" @ONLY)
endif()
