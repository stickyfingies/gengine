#!/usr/bin/bash

trap exit SIGQUIT SIGINT

## Subroutines >>

fetch_dependencies() {
    echo "==Installing vcpkg=="
    if [ ! -d ".vcpkg" ]; then
        git clone https://github.com/Microsoft/vcpkg.git ./.vcpkg
        chmod +x ./.vcpkg/bootstrap-vcpkg.sh
        ./.vcpkg/bootstrap-vcpkg.sh
    fi
    ./.vcpkg/vcpkg install glfw3
    ./.vcpkg/vcpkg install bullet3
    ./.vcpkg/vcpkg install assimp:x64-linux
    ./.vcpkg/vcpkg install assimp:wasm32-emscripten
    ./.vcpkg/vcpkg install glad
    ./.vcpkg/vcpkg install glm:x64-linux
    ./.vcpkg/vcpkg install glm:wasm32-emscripten
    ./.vcpkg/vcpkg install imgui[core,glfw-binding,vulkan-binding]
    ./.vcpkg/vcpkg install vulkan
}

package() {
    zip -r ./dist/pkg/gengine ./dist/bin/ ./data/
}

# << Subroutines

# >> Build script

if [ -z "$1" ]; then
    # ./build.sh
    fetch_dependencies
    cmake --preset linux-vk-app
    cmake --build --preset linux-vk-app
    cmake --preset linux-gl-app
    cmake --build --preset linux-gl-app
    cmake --preset web-gl-app
    cmake --build --preset web-gl-app
    package
else
    case "$1" in
    vcpkg)
        # ./build.sh fetch_dependencies
        fetch_dependencies
        ;;
    dev)
        # ./build.sh dev
        cmake --preset linux
        while :; do
            (watch -n1 -t -g ls -l ./cpp/*) >/dev/null && cmake --build --preset linux
        done
        ;;
    *)
        cmake --preset "$1"
        cmake --build --preset "$1"
        ;;
    esac
fi

# << Build script
