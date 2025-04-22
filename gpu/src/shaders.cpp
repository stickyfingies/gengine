#include "shaders.h"

#include <spirv_glsl.hpp>

#include <shaderc/shaderc.hpp>

#include <iostream>

#ifndef GPU_BACKEND
#define GPU_BACKEND "Undefined"
#endif

using namespace std;
using namespace gpu;

// Convert SPIRV to GLSL ES 300 (WebGL 2)
std::vector<uint32_t> gpu::sprv_to_gles(std::vector<uint32_t> spirv_binary)
{
	spirv_cross::CompilerGLSL glsl(std::move(spirv_binary));

	// Set some options.
	spirv_cross::CompilerGLSL::Options options;
	options.version = 300;
	options.es = true;
	glsl.set_common_options(options);

	// Compile the SPIRV to GLSL.
	std::string source = glsl.compile();

	std::cout << source << std::endl;

	// convert source to vector of uint32_t
	std::vector<uint32_t> glsl_binary;
	glsl_binary.resize(source.size() / sizeof(uint32_t));
	for (size_t i = 0; i < source.size() / sizeof(uint32_t); ++i) {
		glsl_binary[i] = *reinterpret_cast<const uint32_t*>(source.data() + i * sizeof(uint32_t));
	}

	return glsl_binary;
}

// Compiles a shader to a SPIR-V binary. Returns the binary as
// a vector of 32-bit words.
std::vector<uint32_t> gpu::glsl_to_sprv(
	const std::string& source_name,
	gpu::ShaderStage stage,
	const std::string& source,
	bool optimize)
{
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	// Like -DMY_DEFINE=1
	options.AddMacroDefinition("MY_DEFINE", "1");
	if (optimize) {
		options.SetOptimizationLevel(shaderc_optimization_level_size);
	}

	// Add this line to preserve uniform names
	options.SetGenerateDebugInfo();

	shaderc_shader_kind shader_kind = stage == ShaderStage::VERTEX
										  ? shaderc_shader_kind::shaderc_glsl_vertex_shader
										  : shaderc_shader_kind::shaderc_glsl_fragment_shader;

	shaderc::SpvCompilationResult module =
		compiler.CompileGlslToSpv(source, shader_kind, source_name.c_str(), options);

	if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
		std::cerr << module.GetErrorMessage();
		return std::vector<uint32_t>();
	}

	return {module.cbegin(), module.cend()};
}

std::vector<uint32_t>
gpu::compile_shader(gpu::ShaderStage stage, std::string shader_source)
{
	if (strcmp(GPU_BACKEND, "Vulkan") == 0) {
		return gpu::glsl_to_sprv("shader", stage, shader_source);
	}
	else if (strcmp(GPU_BACKEND, "GL") == 0) {
		const auto sprv = gpu::glsl_to_sprv("shader", stage, shader_source);
		return gpu::sprv_to_gles(sprv);
	}
	else {
		std::cerr << "Unknown GPU backend: " << GPU_BACKEND << std::endl;
		return {};
	}
}