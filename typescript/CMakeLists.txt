cmake_minimum_required(VERSION 3.30)

# ./

add_executable(api-ts)

target_include_directories(api-ts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(api-ts PRIVATE core gpu)

set_target_properties(
    api-ts
    PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 23
    CMAKE_CXX_STANDARD_REQUIRED ON
)
target_sources(
    api-ts
    PRIVATE
    main.cpp
)

# vcpkg/DukTape
find_package(
    unofficial-duktape
    CONFIG
    REQUIRED
)
target_link_libraries(
    api-ts
    PRIVATE
    unofficial::duktape::duktape
)

# vcpkg/DukGlue
find_path(DUKGLUE_INCLUDE_DIRS "dukglue/detail_class_proto.h")
target_include_directories(
    api-ts PRIVATE
    ${DUKGLUE_INCLUDE_DIRS}
)
