add_library(core STATIC)

target_link_libraries(core PRIVATE dep_common gpu)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(core
    PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 23
    CMAKE_CXX_STANDARD_REQUIRED ON
)

target_sources(core
    PRIVATE
    main.cpp
    assets.cpp
    physics.cpp
    scene.cpp
    fps_controller.cpp
    stb/stb_image.cpp
)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(core PRIVATE assimp::assimp)

find_package(Bullet CONFIG REQUIRED)
target_link_libraries(core PRIVATE BulletDynamics BulletCollision Bullet3Common LinearMath)
target_link_directories(core PRIVATE ${BULLET_LIBRARY_DIRS})
