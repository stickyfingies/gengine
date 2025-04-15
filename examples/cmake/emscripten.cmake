# Common Emscripten compiler options
add_library(emcc_common INTERFACE)
target_link_options(emcc_common INTERFACE
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
    "-sUSE_GLFW=3"
    "-sUSE_WEBGL2=1"
    "-sMIN_WEBGL_VERSION=2"
    "-sMAX_WEBGL_VERSION=2"
    "-sFULL_ES3"
)

# Debug Emscripten compiler options
add_library(emcc_debug INTERFACE)
target_link_options(emcc_debug INTERFACE
    "-sGL_ASSERTIONS=1"
    "-sERROR_ON_WASM_CHANGES_AFTER_LINK"
    "-sWASM_BIGINT"
    "-O1"
)

# Release Emscripten compiler options
add_library(emcc_release INTERFACE)
target_link_options(emcc_release INTERFACE
    "-sASSERTIONS=0"
    "-O3"
)

# This function configures the Emscripten target with the given name.
function(configure_emscripten_target TARGET_NAME)
    if(NOT EMSCRIPTEN)
        message(FATAL_ERROR "The custom CMake function (configure_emscripten_target) should only be called when EMSCRIPTEN is enabled.")
    endif()

    cmake_parse_arguments(PARSE_ARGV 1
        ARG
        "" # Boolean options
        "OUTPUT_NAME;ASSETS_PATH" # Single value args
        "" # Multi-value args
    )

    if(NOT ARG_OUTPUT_NAME)
        set(ARG_OUTPUT_NAME "index")
    endif()

    if(NOT ARG_ASSETS_PATH)
        set(ARG_ASSETS_PATH "${CMAKE_SOURCE_DIR}/data")
    endif()

    set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME ${ARG_OUTPUT_NAME})

    target_link_options(${TARGET_NAME} PRIVATE "--preload-file" "${ARG_ASSETS_PATH}@data")

    target_link_libraries(${TARGET_NAME} PRIVATE
        embind
        emcc_common
        $<$<CONFIG:Debug>:emcc_debug>
        $<$<CONFIG:Release>:emcc_release>
    )

    configure_file(
        "${CMAKE_SOURCE_DIR}/web/index.html"
        "${CMAKE_BINARY_DIR}/examples/${TARGET_NAME}/index.html"
        @ONLY
    )
endfunction()
