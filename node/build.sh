# Configuration

APP_NAME="linux-vk-app"
GPU_BACKEND="Vulkan"

BUILD_DIR="$(pwd)/artifacts/node-vk-app"
VCPKG_DIR="$(pwd)/dependencies/vcpkg"

CMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake"
CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH};$VCPKG_DIR/installed/x64-linux;$VCPKG_DIR/installed/x64-linux/debug"

cmake-js install

cmake-js build \
    -O $BUILD_DIR \
    --CDCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE" \
    --CDGPU_BACKEND="$GPU_BACKEND"