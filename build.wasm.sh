#!/usr/bin/bash

trap exit SIGQUIT SIGINT

# arguments
while test $# != 0
do
    case "$1" in
    -i|--install) ARG_INSTALL_DEPENDENCIES=t ;;
    -w|--watch) ARG_WATCH=t ;;
    esac
    shift
done

# install package manager
if [ ! -d ".vcpkg" ] || [ ! -z "${ARG_INSTALL_DEPENDENCIES}" ]; then
    echo "==Installing vcpkg=="
    git clone https://github.com/Microsoft/vcpkg.git ./.vcpkg
    chmod +x ./.vcpkg/bootstrap-vcpkg.sh
    ./.vcpkg/bootstrap-vcpkg.sh
    ./.vcpkg/vcpkg install glfw3
    ./.vcpkg/vcpkg install bullet3
    ./.vcpkg/vcpkg install assimp
    ./.vcpkg/vcpkg install glm:x64-linux
    ./.vcpkg/vcpkg install glm:wasm32-emscripten
    ./.vcpkg/vcpkg install imgui[core,glfw-binding,vulkan-binding]
    ./.vcpkg/vcpkg install vulkan
fi

function configure() {
    echo "==Configuring project=="
    cmake -B .build -S . -DCMAKE_TOOLCHAIN_FILE=./.vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=wasm32-emscripten -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=/usr/lib/emscripten/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_CROSSCOMPILING_EMULATOR=/usr/bin/node
}

# build project
function build() {
    echo "==Building project=="
    cmake --build .build
}

# perform the build
if [ -z "${ARG_WATCH}" ]
then
    configure
    build
else
    configure
    while :; do
        (watch -n1 -t -g ls -l ./src/*) > /dev/null && build
    done
fi