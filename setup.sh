#!/usr/bin/bash
cd dependencies

####

echo "==Setup Emscripten=="
./emsdk/emsdk install latest
./emsdk/emsdk activate latest
source ./emsdk/emsdk_env.sh

####

echo "==Setup vcpkg=="
chmod +x ./vcpkg/bootstrap-vcpkg.sh
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install glfw3
./vcpkg/vcpkg install bullet3
./vcpkg/vcpkg install assimp:x64-linux
./vcpkg/vcpkg install assimp:wasm32-emscripten
./vcpkg/vcpkg install glad
./vcpkg/vcpkg install glm:x64-linux
./vcpkg/vcpkg install glm:wasm32-emscripten
./vcpkg/vcpkg install imgui[core,glfw-binding,vulkan-binding]
./vcpkg/vcpkg install vulkan
