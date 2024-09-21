#!/usr/bin/sh

echo "==Setup Emscripten=="

alias emsdk=./dependencies/emsdk/emsdk
emsdk install latest
emsdk activate latest
source ./dependencies/emsdk/emsdk_env.sh

echo "==Setup vcpkg=="

chmod +x ./dependencies/vcpkg/bootstrap-vcpkg.sh
./dependencies/vcpkg/bootstrap-vcpkg.sh

alias vcpkg=./dependencies/vcpkg/vcpkg

vcpkg install glm:wasm32-emscripten
vcpkg install glm:wasm32-emscripten
vcpkg install glm:wasm32-emscripten
vcpkg install glm:wasm32-emscripten
vcpkg install glm:wasm32-emscripten
vcpkg install glm:wasm32-emscripten

vcpkg install glfw3
vcpkg install bullet3
vcpkg install assimp:x64-linux
vcpkg install assimp:wasm32-emscripten
vcpkg install glad
vcpkg install glm:x64-linux
vcpkg install glm:wasm32-emscripten
vcpkg install imgui[core,glfw-binding,vulkan-binding]
vcpkg install vulkan
