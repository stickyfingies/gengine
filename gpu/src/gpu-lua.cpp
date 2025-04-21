/**
 * @file gpu-lua.cpp
 * @brief Demo using Lua to script the GPU interface
 * Reference: https://github.com/ThePhD/sol2/blob/develop/examples/source/usertype.cpp
 */

#include "gpu.h"
#include <GLFW/glfw3.h>
#include <iostream>

#include <sol/sol.hpp>

class Window {
public:
	Window(int width, int height, const char* title)
	{
		window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	}

	bool shouldClose() const { return glfwWindowShouldClose(window); }
	void pollEvents() const { glfwPollEvents(); }

	~Window() { glfwDestroyWindow(window); }

    GLFWwindow* getRawWindowPtr() const { return window; }

private:
	GLFWwindow* window;
};

class RenderDevice {
public:
    RenderDevice(Window& window)
    {
        // Initialize the GPU device with the given window
    }

    ~RenderDevice()
    {
        // Cleanup GPU resources
    }
};

int main()
{
	glfwInit();
    gpu::configure_glfw();

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<Window>(
		"Window",
		sol::constructors<Window(int, int, const char*)>(),
		"shouldClose",
		&Window::shouldClose,
		"pollEvents",
		&Window::pollEvents);

    lua.new_usertype<RenderDevice>(
        "RenderDevice",
        sol::constructors<RenderDevice(Window&)>());

	// Open script.lua
	lua.script_file("script.lua");

	glfwTerminate();

	return 0;
}