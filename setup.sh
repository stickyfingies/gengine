#!/usr/bin/sh
set -eou pipefail
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}LOCAL OPERATION ONLY: No system modifications will be made.${NC}"
echo -e "${BLUE}All file changes are confined to this directory.${NC}"

#### Directory Index
DIR_GENGINE=$(dirname $(realpath "$0"))
DIR_EMSDK="${DIR_GENGINE}/dependencies/emsdk"
DIR_VCPKG="${DIR_GENGINE}/dependencies/vcpkg"
DIR_PORTS="${DIR_GENGINE}/dependencies/vcpkg-ports"

#### Git Submodules (init & update)
if git submodule status | grep -q '^[+-]'; then
    echo -e "${BLUE}git submodule update --init --recursive${NC}"
    git submodule update --init --recursive
fi

#### Install Emscripten SDK
cd ${DIR_EMSDK}
git switch main
git pull
./emsdk install latest
./emsdk activate
chmod +x ./emsdk_env.sh
source ./emsdk_env.sh

#### Install Vcpkg
cd ${DIR_VCPKG}
git switch master
git pull
chmod +x ./bootstrap-vcpkg.sh
sh ./bootstrap-vcpkg.sh -disableMetrics # you evil f^#ks
./vcpkg update
./vcpkg install assimp:x64-linux --overlay-ports=${DIR_PORTS}/assimp
./vcpkg install assimp:wasm32-emscripten --overlay-ports=${DIR_PORTS}/assimp
./vcpkg install bullet3:x64-linux
./vcpkg install bullet3:wasm32-emscripten
./vcpkg install dukglue
./vcpkg install duktape
./vcpkg install glad
./vcpkg install glfw3
./vcpkg install glm:x64-linux
./vcpkg install glm:wasm32-emscripten
./vcpkg install imgui[core,glfw-binding,vulkan-binding]
./vcpkg install lua:x64-linux
./vcpkg install lua:wasm32-emscripten
./vcpkg install reflectcpp:x64-linux
./vcpkg install reflectcpp:wasm32-emscripten
./vcpkg install shaderc:x64-linux
./vcpkg install sol2:x64-linux
./vcpkg install sol2:wasm32-emscripten
./vcpkg install spirv-cross:x64-linux
./vcpkg install spirv-reflect:x64-linux
./vcpkg install vulkan
