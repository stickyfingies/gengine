/**
 * @file gpu-lua.cpp
 * @brief Demo using Lua to script the GPU interface
 * Reference: https://github.com/ThePhD/sol2/blob/develop/examples/source/usertype.cpp
 */

#include "gpu.h"
#include "shaders.h"
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

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
		"destroy_buffer",
		&gpu::RenderDevice::destroy_buffer,
		"destroy_pipeline",
		&gpu::RenderDevice::destroy_pipeline,
		"create_geometry",
		&gpu::RenderDevice::create_geometry,
		"destroy_geometry",
		&gpu::RenderDevice::destroy_geometry,
		"simple_draw",
		&gpu::RenderDevice::simple_draw);

	lua["RenderDevice"]["create_buffer"] = sol::overload(
		&gpu::RenderDevice::create_buffer,
		[](gpu::RenderDevice* device,
		   gpu::BufferUsage usage,
		   std::size_t stride,
		   std::size_t element_count,
		   std::vector<float> data) {
			// if it's an index buffer, convert data to uint32_t
			if (usage == gpu::BufferUsage::INDEX) {
				std::cout << "Creating uint32_t index buffer" << std::endl;
				std::vector<uint32_t> uint_data(data.size());
				for (size_t i = 0; i < data.size(); ++i) {
					uint_data[i] = static_cast<uint32_t>(data[i]);
				}
				return device->create_buffer(usage, stride, element_count, uint_data.data());
			}

			return device->create_buffer(usage, stride, element_count, data.data());
		});

	lua["RenderDevice"]["create_pipeline"] = [](gpu::RenderDevice* device,
												const std::vector<uint32_t>& vert_code_bytes,
												const std::vector<uint32_t>& frag_code_bytes,
												std::vector<gpu::VertexAttribute> vertex_attributes,
												gpu::WindingOrder winding_order =
													gpu::WindingOrder::COUNTERCLOCKWISE) {
		// STL containers only work in Sol2 with values, not references

		// convert vert_code_bytes to std::string
		std::string vert_code(
			reinterpret_cast<const char*>(vert_code_bytes.data()),
			vert_code_bytes.size() * sizeof(uint32_t));
		// convert frag_code_bytes to std::string
		std::string frag_code(
			reinterpret_cast<const char*>(frag_code_bytes.data()),
			frag_code_bytes.size() * sizeof(uint32_t));

		return device->create_pipeline(vert_code, frag_code, vertex_attributes, winding_order);
	};

	// Enums
	lua["BufferUsage"] =
		lua.create_table_with("VERTEX", gpu::BufferUsage::VERTEX, "INDEX", gpu::BufferUsage::INDEX);
	lua["VertexAttribute"] = lua.create_table_with(
		"VEC3", gpu::VertexAttribute::VEC3_FLOAT, "VEC2", gpu::VertexAttribute::VEC2_FLOAT);
	lua["WindingOrder"] = lua.create_table_with(
		"CLOCKWISE",
		gpu::WindingOrder::CLOCKWISE,
		"COUNTERCLOCKWISE",
		gpu::WindingOrder::COUNTERCLOCKWISE);
	lua["ShaderStage"] = lua.create_table_with(
		"VERTEX", gpu::ShaderStage::VERTEX, "FRAGMENT", gpu::ShaderStage::FRAGMENT);

	// Functions
	lua.set_function("shouldClose", [&]() -> bool {
		return static_cast<bool>(glfwWindowShouldClose(window.get()));
	});
	lua.set_function("pollEvents", [&]() { glfwPollEvents(); });
	lua.set_function("swapBuffers", [&]() { glfwSwapBuffers(window.get()); });
	lua["getData"] = []() -> void* { return nullptr; };
	lua["open_file"] = &open_file;
	lua["sprv_to_gles"] = &gpu::sprv_to_gles;
	lua["glsl_to_sprv"] = &gpu::glsl_to_sprv;
	lua["compile_shader"] = &gpu::compile_shader;

	// Globals
	lua["gpu"] = std::move(render_device);

	lua.script_file("script.lua");

	glfwTerminate();

	return 0;
}