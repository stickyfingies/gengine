# The target 'node' is a node.js addon

add_library(node SHARED)

# From Cmake.js documentation
add_definitions(-DNAPI_VERSION=4)
include_directories(${CMAKE_JS_INC})

target_link_libraries(node PRIVATE ${CMAKE_JS_LIB} dep_common)

target_include_directories(node PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_JS_INC})

set_target_properties(node
    PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 23
    CMAKE_CXX_STANDARD_REQUIRED ON
    PREFIX "" SUFFIX ".node"
)

target_sources(node PRIVATE ${CMAKE_JS_SRC} main.cpp)

## [Reference] ##
# find_package(Vulkan REQUIRED)
# target_link_libraries(gpu PRIVATE ${Vulkan_LIBRARIES})
# target_include_directories(gpu PRIVATE ${Vulkan_INCLUDE_DIRS})
