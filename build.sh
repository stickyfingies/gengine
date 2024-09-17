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
    ./.vcpkg/vcpkg install assimp
    ./.vcpkg/vcpkg install glad
    ./.vcpkg/vcpkg install glm:x64-linux
    ./.vcpkg/vcpkg install glm:wasm32-emscripten
    ./.vcpkg/vcpkg install imgui[core,glfw-binding,vulkan-binding]
    ./.vcpkg/vcpkg install vulkan
}

cmake_configure_linux() {
    cmake -B .cmake -S . -DCMAKE_TOOLCHAIN_FILE=./.vcpkg/scripts/buildsystems/vcpkg.cmake
}

cmake_configure_web() {
    cmake -B .cmake -S . -DCMAKE_TOOLCHAIN_FILE=./.vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=wasm32-emscripten -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=/usr/lib/emscripten/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_CROSSCOMPILING_EMULATOR=/usr/bin/node
}

build() {
    cmake --build .cmake
}

package() {
    zip -r ./dist/pkg/gengine ./dist/bin/ ./data/
}

# << Subroutines

# >> Build script

if [ $# -eq 0 ]; then
    # ./build.sh
    fetch_dependencies
    rm -rf .cmake
    cmake_configure_linux
    build
    rm -rf .cmake
    cmake_configure_web
    build
    package
fi

while test $# != 0; do
    case "$1" in
    linux)
        # ./build.sh linux
        cmake_configure_linux
        build
        ;;
    web)
        # ./build.sh web
        cmake_configure_web
        build
        ;;
    vcpkg)
        # ./build.sh fetch_dependencies
        fetch_dependencies
        ;;
    dev)
        # ./build.sh dev
        cmake_configure_linux
        while :; do
            (watch -n1 -t -g ls -l ./cpp/*) >/dev/null && build
        done
        ;;
    esac
    shift
done

# << Build script
