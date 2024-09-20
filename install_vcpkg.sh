#!/usr/bin/sh

echo "==Installing vcpkg=="

if [ ! -d ".vcpkg" ]; then
    git clone https://github.com/Microsoft/vcpkg.git ./.vcpkg
    chmod +x ./.vcpkg/bootstrap-vcpkg.sh
    ./.vcpkg/bootstrap-vcpkg.sh
fi

alias vcpkg=./.vcpkg/vcpkg

vcpkg install glfw3
vcpkg install bullet3
vcpkg install assimp:x64-linux
vcpkg install assimp:wasm32-emscripten
vcpkg install glad
vcpkg install glm:x64-linux
vcpkg install glm:wasm32-emscripten
vcpkg install imgui[core,glfw-binding,vulkan-binding]
vcpkg install vulkan
