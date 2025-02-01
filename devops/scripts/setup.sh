#!/bin/bash

# Source the common script
source "$(dirname "$0")/common.sh"

# Directory for dev tools (emsdk, vcpkg)
DEV_TOOLS_DIR="${GIT_ROOT}/devops/tools"

# Create the dev tools directory
mkdir -p "${DEV_TOOLS_DIR}"
cd "${DEV_TOOLS_DIR}"

# Clone and set up Emscripten (emsdk)
log "info" "Setting up Emscripten (emsdk)..."
if [[ ! -d "${DEV_TOOLS_DIR}/emsdk" ]]; then
    run_command "Cloning emsdk repository..." "git clone https://github.com/emscripten-core/emsdk.git"
fi
cd "${DEV_TOOLS_DIR}/emsdk"
run_command "Updating emsdk repository..." "git pull"
run_command "Installing latest emsdk..." "./emsdk install latest"
run_command "Activating latest emsdk..." "./emsdk activate latest"

# Add emsdk to the PATH
export EMSDK="${DEV_TOOLS_DIR}/emsdk"
export EMSCRIPTEN="${EMSDK}/upstream/emscripten"
export PATH="${EMSDK}:${EMSCRIPTEN}:${PATH}"

# Return to the dev tools directory
cd "${DEV_TOOLS_DIR}"

# Clone and set up vcpkg
log "info" "Setting up vcpkg..."
if [[ ! -d "${DEV_TOOLS_DIR}/vcpkg" ]]; then
    run_command "Cloning vcpkg repository..." "git clone https://github.com/microsoft/vcpkg.git"
fi
cd "${DEV_TOOLS_DIR}/vcpkg"
run_command "Updating vcpkg repository..." "git pull"
run_command "Bootstrapping vcpkg..." "./bootstrap-vcpkg.sh"

# Add vcpkg to the PATH
export VCPKG_ROOT="${DEV_TOOLS_DIR}/vcpkg"
export PATH="${VCPKG_ROOT}:${PATH}"

# Verify installations
log "info" "Verifying installations..."
if ! command_exists emcc; then
    exit_with_error "emcc not found. Emscripten setup failed."
fi
if ! command_exists vcpkg; then
    exit_with_error "vcpkg not found. vcpkg setup failed."
fi

emcc --version
vcpkg --version

log "success" "Environment setup completed successfully!"
log "success" "Emscripten and vcpkg are ready to use."

# Configure vcpkg overlay ports directory
VCPKG_PORTS_DIR="${GIT_ROOT}/devops/config/vcpkg/ports"

# Install dependencies with vcpkg
cd "${GIT_ROOT}/devops/tools"
log "info" "Installing vcpkg dependencies..."
./vcpkg/vcpkg install assimp:x64-linux --overlay-ports=${VCPKG_PORTS_DIR}
./vcpkg/vcpkg install assimp:wasm32-emscripten --overlay-ports=${VCPKG_PORTS_DIR}
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

log "success" "vcpkg dependencies installed successfully!"