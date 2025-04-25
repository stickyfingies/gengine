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

struct ShaderDataType {
	std::string name;
	std::string base_type;
	int array_elements;
	int array_stride;
	int size;
};

struct ShaderVariable {
	std::string name;
	std::string format;
	int location;
	ShaderDataType type;
};

struct BufferMemberLayout {
	std::string name;
	uint32_t offset;
	uint32_t size;
};

struct UniformBufferLayout {
	uint32_t set;
	uint32_t binding;
	std::string block_name;
	uint32_t block_size;
	std::vector<BufferMemberLayout> members;
};

struct ShaderMetaData {
	std::string shader_name;
	std::string shader_stage;
	std::string entry_point_name;
	std::vector<ShaderVariable> input_variables;
	std::vector<ShaderVariable> output_variables;
};

// This is an intermediate stage ABOVE THE RENDERDEVICE that contains
// compiled shader code and vertex attributes.
struct ShaderObject {
	std::vector<uint32_t> target_vertex_shader;	  // SPIR-V or GLSL-ES
	std::vector<uint32_t> target_fragment_shader; // SPIR-V or GLSL-ES
	std::vector<gpu::VertexAttribute> vertex_attributes;
};

std::vector<uint32_t> glsl_to_sprv(
	const std::string& source_name,
	gpu::ShaderStage stage,
	const std::string& source,
	bool optimize);

std::vector<uint32_t> sprv_to_gles(std::vector<uint32_t> spirv_binary);

gpu::ShaderMetaData reflect_shader(std::vector<uint32_t> spirv_blob);

ShaderObject compile_shaders(std::string vert_source, std::string frag_source);

} // namespace gpu