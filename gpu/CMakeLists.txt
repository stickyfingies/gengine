# The target 'gpu' is our cross-platform rendering library.

# The interface gpu.h is common across implementations,
# but the .cpp implementations depend on what platform we're
# comipling the library for.

add_library(gpu STATIC)

target_link_libraries(gpu PRIVATE dep_common)

target_include_directories(gpu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

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
        gpu.vulkan.cpp
    )
    find_package(Vulkan REQUIRED)
    target_link_libraries(gpu PRIVATE ${Vulkan_LIBRARIES})
    target_include_directories(gpu PRIVATE ${Vulkan_INCLUDE_DIRS})
    # End Vulkan implementation
elseif(GPU_BACKEND MATCHES GL)
    # Begin OpenGL ES / WebGL implementation
    target_sources(gpu
        PRIVATE
        gpu.opengl.cpp
    )

    # Glad is used for OpenGL ES, but is not needed for WebGL.
    if(CMAKE_SYSTEM_NAME MATCHES Linux)
        find_package(glad CONFIG REQUIRED)
        target_link_libraries(gpu PRIVATE glad::glad)
    endif()
    # End OpenGL ES / WebGL implementation
endif()
