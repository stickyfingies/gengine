# WIP MVP
# This script compiles 'main.cpp' into a .node native binding for Node.JS

APP_NAME="linux-vk-app"
GPU_BACKEND="Vulkan"

ROOT_DIR="$(pwd)/.."
BUILD_DIR="$ROOT_DIR/artifacts/node-vk-app"
VCPKG_DIR="$ROOT_DIR/dependencies/vcpkg"

CMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake"
CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH};$VCPKG_DIR/installed/x64-linux;$VCPKG_DIR/installed/x64-linux/debug"

cmake-js install

cmake-js build \
    -O $BUILD_DIR \
    --CDCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE" \
    --CDGPU_BACKEND="$GPU_BACKEND"