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
	lua.open_libraries(sol::lib::base, sol::lib::package);

	// Structures
	lua.new_usertype<gpu::BufferHandle>("BufferHandle");
	lua.new_usertype<gpu::ShaderObject>(
		"ShaderObject",
		"target_vertex_shader",
		&gpu::ShaderObject::target_vertex_shader,
		"target_fragment_shader",
		&gpu::ShaderObject::target_fragment_shader,
		"vertex_attributes",
		&gpu::ShaderObject::vertex_attributes);
	lua.new_usertype<gpu::RenderDevice>(
		"RenderDevice",
		"destroy_buffer",
		&gpu::RenderDevice::destroy_buffer,
		"create_pipeline",
		&gpu::RenderDevice::create_pipeline,
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
			// Overload so we can pass a table of numbers from Lua
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
	lua["compile_shaders"] = &gpu::compile_shaders;

	// Globals
	lua["gpu"] = std::move(render_device);

	lua.script_file("script.lua");

	glfwTerminate();

	return 0;
}