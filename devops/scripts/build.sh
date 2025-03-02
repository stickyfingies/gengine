#!/bin/bash
source "$(dirname "$0")/common.sh"

# Reference: CMakePresets.json
build_presets=()
build_presets+=( "linux-vk-dev" "Configure and build for Linux (Vulkan, Debug)" )
build_presets+=( "linux-vk-app" "Configure and build for Linux (Vulkan, Release)" )
build_presets+=( "linux-gl-dev" "Configure and build for Linux (OpenGL, Debug)" )
build_presets+=( "linux-gl-app" "Configure and build for Linux (OpenGL, Release)" )
build_presets+=( "web-gl-dev" "Configure and build for Web (WebGL, Debug)" )
build_presets+=( "web-gl-app" "Configure and build for Web (WebGL, Release)" )

# Function: display usage and exit
usage() {
    echo -e "${BOLD}Usage:${NC} $0 <preset>"
    echo "Available presets:"
    for (( i=0; i<${#build_presets[@]}; i+=2 )); do
        local name="${build_presets[$i]}"
        local description="${build_presets[$i+1]}"
        echo -e "  $name\t  # $description"
    done
    exit 1
}

# Enforce a valid preset argument
if [[ $# -eq 0 ]]; then
    usage
else
    # Get the preset argument
    PRESET="$1"
    shift
    
    # Validate the preset argument using the set of valid build presets
    preset_valid=0
    for (( i = 0; i < ${#build_presets[@]}; i += 2 )); do
        preset_name="${build_presets[$i]}"
        if [[ "${PRESET}" == "${preset_name}" ]]; then
            preset_valid=1
            break
        fi
    done

    # Enforce a valid preset argument
    if [[ "${preset_valid}" -eq 0 ]]; then 
        log "error" "Invalid preset: ${PRESET}"
        usage
    fi
fi

# Configure, build, and package based on the preset
log "info" "Building project with preset: ${PRESET}"

cd "${GIT_ROOT}/source" || exit

# Step 1: Configure
log "info" "Configuring project..."
cmake --preset="${PRESET}" || exit_with_error "Configuration failed."

# Step 2: Build
log "info" "Building project..."
cmake --build --preset="${PRESET}" || exit_with_error "Build failed."

# Step 3: Package (if applicable)
if [[ "$PRESET" == *"-app" ]]; then
    log "info" "Packaging project..."
    cpack --preset="${PRESET}" || exit_with_error "Packaging failed."
fi

log "success" "Build completed successfully!"