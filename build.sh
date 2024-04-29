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
if [ ! -d "vcpkg" ] || [ ! -z "${ARG_INSTALL_DEPENDENCIES}" ]; then
    echo "==Installing vcpkg=="
    git clone https://github.com/Microsoft/vcpkg.git
    chmod +x ./vcpkg/bootstrap-vcpkg.sh
    ./vcpkg/bootstrap-vcpkg.sh
    ./vcpkg/vcpkg install glfw3
    ./vcpkg/vcpkg install bullet3
    ./vcpkg/vcpkg install assimp
fi

function configure() {
    echo "==Configuring project=="
    cmake -B build -S .
}

# build project
function build() {
    echo "==Building project=="
    cmake --build build
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