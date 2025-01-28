#!/usr/bin/env bash
# This script builds the whole project from square one.

git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
VCPKG_ROOT=$(pwd)/vcpkg
./vcpkg/vcpkg install glad
./vcpkg/vcpkg install glfw3
./vcpkg/vcpkg install glm
./vcpkg/vcpkg install imgui[core,glfw-binding,vulkan-binding]
./vcpkg/vcpkg install vulkan

mkdir -p build
cd build
CMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
CMAKE_INSTALL_PREFIX=$(pwd)/../dist
cmake .. -DGPU_BACKEND=GL -DCMAKE_INSTALL_PREFIX=$CMAKE_INSTALL_PREFIX -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE
cmake --build .
cmake --build . --target install
cmake .. -DGPU_BACKEND=Vulkan -DCMAKE_INSTALL_PREFIX=$CMAKE_INSTALL_PREFIX -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE
cmake --build .
cmake --build . --target install