#pragma once

#include "gpu.h"

#include <string>
#include <vector>

namespace gpu {

// Convert SPIRV to GLSL ES 300 (WebGL 2)
std::vector<uint32_t> sprv_to_gles(std::vector<uint32_t> spirv_binary);

// Compiles a GLSL shader to SPIR-V binary format
std::vector<uint32_t> glsl_to_sprv(
	const std::string& source_name,
	gpu::ShaderStage kind,
	const std::string& source,
	bool optimize = false);

// Compiles a GLSL shader into the appropriate shader format for this GPU backend
// (SPIR-V for Vulkan, GLSL-ES for OpenGL)
std::vector<uint32_t> compile_shader(gpu::ShaderStage stage, std::string shader_source);

} // namespace gpu