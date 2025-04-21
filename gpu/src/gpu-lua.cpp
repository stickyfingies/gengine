/**
 * @file gpu-lua.cpp
 * @brief Demo using Lua to script the GPU interface
 * Reference: https://github.com/ThePhD/sol2/blob/develop/examples/source/usertype.cpp
 */

#include "gpu.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>

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

int main()
{
	glfwInit();
	gpu::configure_glfw();

	shared_ptr<GLFWwindow> window = createSharedGlfwWindow(1280, 720, "GPU Lua Demo");
	unique_ptr<gpu::RenderDevice> render_device = gpu::RenderDevice::create(window);

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<gpu::BufferHandle>("BufferHandle");
	lua.new_usertype<gpu::RenderDevice>(
		"RenderDevice",
		"create_buffer",
		&gpu::RenderDevice::create_buffer,
		"destroy_buffer",
		&gpu::RenderDevice::destroy_buffer);

	lua["BufferUsage"] =
		lua.create_table_with("VERTEX", gpu::BufferUsage::VERTEX, "INDEX", gpu::BufferUsage::INDEX);

	lua.set_function("shouldClose", [&]() -> bool {
		return static_cast<bool>(glfwWindowShouldClose(window.get()));
	});

	lua.set_function("pollEvents", [&]() { glfwPollEvents(); });

	lua["getData"] = []() -> void* { return nullptr; };

	lua["gpu"] = std::move(render_device);

	lua.script_file("script.lua");

	glfwTerminate();

	return 0;
}