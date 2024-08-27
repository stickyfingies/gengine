#!/usr/bin/bash

############################
# GENGINE BUILD AUTOMATION #
############################

trap exit SIGQUIT SIGINT

###############
#  CLI Config  #
#  @ARG_DOCS  #
while test $# != 0
do
    case "$1" in
    -i|--install) ARG_INSTALL_DEPENDENCIES=t ;;
    -w|--watch) ARG_WATCH=t ;;
    esac
    shift
done

# [Ctrl F] for @ARG_DOCS.
if [ ! -d ".vcpkg" ] || [ ! -z "${ARG_INSTALL_DEPENDENCIES}" ]; then
    echo "==Installing vcpkg=="
    git clone https://github.com/Microsoft/vcpkg.git ./.vcpkg
    chmod +x ./.vcpkg/bootstrap-vcpkg.sh
    ./.vcpkg/bootstrap-vcpkg.sh
    ./.vcpkg/vcpkg install glfw3
    ./.vcpkg/vcpkg install bullet3
    ./.vcpkg/vcpkg install assimp
    ./.vcpkg/vcpkg install glm
    ./.vcpkg/vcpkg install imgui[core,glfw-binding,vulkan-binding]
    ./.vcpkg/vcpkg install vulkan
fi

function sdk_configure() {
    echo "==Configuring project=="
    cmake -B .build -S .
}

# build project
function sdk_build() {
    echo "==Building project=="
    cmake --build .build
}

# package project
function sdk_package() {
    chrpath -r dist dist/gengine
    zip -r dist/gengine.zip dist data
}

# [Ctrl F] for @ARG_DOCS.
if [ -z "${ARG_WATCH}" ]
then # source watching is [ENABLED]
    sdk_configure
    sdk_build
    sdk_package
else # source watching is [DISABLED]
    sdk_configure
    while :; do
        (watch -n1 -t -g ls -l ./src/*) > /dev/null && sdk_build
    done
fi