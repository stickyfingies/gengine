#!/usr/bin/bash
cd dependencies


echo ">> EMSDK"
####
./emsdk/emsdk install latest
./emsdk/emsdk activate latest
source ./emsdk/emsdk_env.sh


echo ">> VCPKG"
####
VCPKG_BOOTSTRAP_SCRIPT=./vcpkg/bootstrap-vcpkg.sh
chmod +x $VCPKG_BOOTSTRAP_SCRIPT && $VCPKG_BOOTSTRAP_SCRIPT
./vcpkg/vcpkg install assimp:x64-linux --overlay-ports=vcpkg-ports/assimp
./vcpkg/vcpkg install assimp:wasm32-emscripten --overlay-ports=vcpkg-ports/assimp
./vcpkg/vcpkg install bullet3:x64-linux
./vcpkg/vcpkg install bullet3:wasm32-emscripten
./vcpkg/vcpkg install dukglue
./vcpkg/vcpkg install duktape
./vcpkg/vcpkg install glad
./vcpkg/vcpkg install glfw3
./vcpkg/vcpkg install glm:x64-linux
./vcpkg/vcpkg install glm:wasm32-emscripten
./vcpkg/vcpkg install imgui[core,glfw-binding,vulkan-binding]
./vcpkg/vcpkg install vulkan