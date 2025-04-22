/**
 * Shaders.h defines this semblance of an interface where advanced shader
 * processing happens above the gpu::RenderDevice by reflecting on the source,
 * and the resulting structure should be passed into the RenderDevice using
 * the existing standard interfaces wherever possible.
 */

#pragma once

#include "gpu.h"

#include <string>
#include <vector>

namespace gpu {

// This is an intermediate stage ABOVE THE RENDERDEVICE that contains
// compiled shader code and vertex attributes.
struct ShaderObject {
	std::vector<uint32_t> target_vertex_shader; // SPIR-V or GLSL-ES
	std::vector<uint32_t> target_fragment_shader; // SPIR-V or GLSL-ES
	std::vector<gpu::VertexAttribute> vertex_attributes;
};

ShaderObject compile_shaders(std::string vert_source, std::string frag_source);

} // namespace gpu