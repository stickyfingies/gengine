/**
 * @file gpu-lua.cpp
 * @brief Demo using Lua to script the GPU interface
 * Reference: https://github.com/ThePhD/sol2/blob/develop/examples/source/usertype.cpp
 */

#include "gpu.h"
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include <spirv_glsl.hpp>

#include <shaderc/shaderc.hpp>

#undef SOL_SAFE_NUMERICS
#include <sol/sol.hpp>

using namespace std;

struct GlfwWindowDeleter {
	void operator()(GLFWwindow* window) const
	{
		if (window) {
			glfwDestroyWindow(window);
		}
	}
};

static shared_ptr<GLFWwindow> createSharedGlfwWindow(
	int width,
	int height,
	const char* title,
	GLFWmonitor* monitor = nullptr,
	GLFWwindow* share = nullptr)
{
	GLFWwindow* window = glfwCreateWindow(width, height, title, monitor, share);
	return shared_ptr<GLFWwindow>(window, GlfwWindowDeleter());
}

static string open_file(const char* filename)
{
	ifstream file(filename);
	if (!file.is_open()) {
		cerr << "Failed to open file: " << filename << endl;
		return "";
	}

	string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
	file.close();
	return content;
}

// Convert SPIRV to GLSL ES 300 (WebGL 2)
static std::string spvglsl(std::string spirv_binary_src)
{
	// Convert spirv_binary_src to vector of uint32_t
	std::vector<uint32_t> spirv_binary;
	spirv_binary.reserve(spirv_binary_src.size() / sizeof(uint32_t));
	for (size_t i = 0; i < spirv_binary_src.size(); i += sizeof(uint32_t)) {
		spirv_binary.push_back(*reinterpret_cast<const uint32_t*>(spirv_binary_src.data() + i));
	}

	spirv_cross::CompilerGLSL glsl(std::move(spirv_binary));

	// Set some options.
	spirv_cross::CompilerGLSL::Options options;
	options.version = 300;
	options.es = true;
	glsl.set_common_options(options);

	// Compile to GLSL, ready to give to GL driver.
	std::string source = glsl.compile();

	return source;
}

// Compiles a shader to a SPIR-V binary. Returns the binary as
// a vector of 32-bit words.
std::vector<uint32_t> compile_file(
	const std::string& source_name,
	shaderc_shader_kind kind,
	const std::string& source,
	bool optimize = false)
{
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	// Like -DMY_DEFINE=1
	options.AddMacroDefinition("MY_DEFINE", "1");
	if (optimize) {
		options.SetOptimizationLevel(shaderc_optimization_level_size);
	}

	shaderc::SpvCompilationResult module =
		compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);

	if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
		std::cerr << module.GetErrorMessage();
		return std::vector<uint32_t>();
	}

	return {module.cbegin(), module.cend()};
}

int main()
{
	glfwInit();
	gpu::configure_glfw();

	shared_ptr<GLFWwindow> window = createSharedGlfwWindow(1280, 720, "GPU Lua Demo");
	unique_ptr<gpu::RenderDevice> render_device = gpu::RenderDevice::create(window);

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	// Structures
	lua.new_usertype<gpu::BufferHandle>("BufferHandle");
	lua.new_usertype<gpu::RenderDevice>(
		"RenderDevice",
		"create_buffer",
		&gpu::RenderDevice::create_buffer,
		"destroy_buffer",
		&gpu::RenderDevice::destroy_buffer,
		"destroy_pipeline",
		&gpu::RenderDevice::destroy_pipeline);

	lua["RenderDevice"]["create_pipeline"] =
		[](gpu::RenderDevice* device,
		   const std::vector<uint32_t>& vert_code_bytes,
		   const std::vector<uint32_t>& frag_code_bytes,
		   std::vector<gpu::VertexAttribute> vertex_attributes) {
			// STL containers only work in Sol2 with values, not references

			// convert vert_code_bytes to std::string
			std::string vert_code(reinterpret_cast<const char*>(vert_code_bytes.data()), vert_code_bytes.size() * sizeof(uint32_t));
			// convert frag_code_bytes to std::string
			std::string frag_code(reinterpret_cast<const char*>(frag_code_bytes.data()), frag_code_bytes.size() * sizeof(uint32_t));

			return device->create_pipeline(vert_code, frag_code, vertex_attributes);
		};

	// Enums
	lua["BufferUsage"] =
		lua.create_table_with("VERTEX", gpu::BufferUsage::VERTEX, "INDEX", gpu::BufferUsage::INDEX);
	lua["VertexAttribute"] = lua.create_table_with(
		"VEC3", gpu::VertexAttribute::VEC3_FLOAT, "VEC2", gpu::VertexAttribute::VEC2_FLOAT);
	lua["ShaderC"] = lua.create_table_with(
		"VERTEX",
		shaderc_shader_kind::shaderc_glsl_vertex_shader,
		"FRAGMENT",
		shaderc_shader_kind::shaderc_glsl_fragment_shader);

	// Functions
	lua.set_function("shouldClose", [&]() -> bool {
		return static_cast<bool>(glfwWindowShouldClose(window.get()));
	});
	lua.set_function("pollEvents", [&]() { glfwPollEvents(); });
	lua["getData"] = []() -> void* { return nullptr; };
	lua["open_file"] = &open_file;
	lua["spvglsl"] = &spvglsl;
	lua["compile_file"] = &compile_file;

	// Globals
	lua["gpu"] = std::move(render_device);

	lua.script_file("script.lua");

	glfwTerminate();

	return 0;
}