/**
 * @file kernel.cpp - implements the engine lifecycle for Native and Web platforms.
 */

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "kernel.h"
#include "window.h"
#include "world.h"

#include <gpu.h>

#include <GLFW/glfw3.h>

#include <iostream>
#include <memory>

using namespace gengine;
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

static void mouse_callback(GLFWwindow* window, double pos_x, double pos_y)
{
	auto window_data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

	window_data->delta_mouse_x = pos_x - window_data->mouse_x;
	window_data->delta_mouse_y = window_data->mouse_y - pos_y;

	window_data->mouse_x = pos_x;
	window_data->mouse_y = pos_y;
}

struct EngineKernel {
	WindowData window_data;
	shared_ptr<gpu::RenderDevice> renderer;
	unique_ptr<World> world;
	shared_ptr<GLFWwindow> window;
	size_t frame_count;
	double last_displayed_fps;
	double last_time;
	float ms_per_frame = 0.0f;
};

EngineKernel* kernel_create(bool editor_enabled)
{
	EngineKernel* kernel = new EngineKernel;

	glfwInit();

	gpu::configure_glfw();

	kernel->window = createSharedGlfwWindow(280, 720, "Gengine", nullptr, nullptr);

	glfwSetWindowUserPointer(kernel->window.get(), &kernel->window_data);

	if (!editor_enabled) {
		glfwSetInputMode(kernel->window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetCursorPosCallback(kernel->window.get(), mouse_callback);
	}

	// << Window startup

	// >> System startup

	kernel->renderer = gpu::RenderDevice::create(kernel->window);

	kernel->world = World::create(kernel->window, kernel->renderer);

	// << System startup

	// >> Main loop

	kernel->last_displayed_fps = glfwGetTime();
	kernel->frame_count = 0u;
	kernel->last_time = glfwGetTime();
	kernel->ms_per_frame = 0.0f;

	// << Main loop

	return kernel;
}

bool kernel_running(EngineKernel* kernel) { return glfwWindowShouldClose(kernel->window.get()) == false; }

void kernel_update(EngineKernel* kernel)
{
	++kernel->frame_count;

	const auto current_time = glfwGetTime();
	const auto elapsed_time = current_time - kernel->last_time;

	// ms/frame
	if (glfwGetTime() - kernel->last_displayed_fps >= 1.0) {
		kernel->ms_per_frame = 1000.0 / kernel->frame_count;
		kernel->last_displayed_fps = glfwGetTime();
		kernel->frame_count = 0;
	}

	glfwPollEvents();

	if (glfwGetKey(kernel->window.get(), GLFW_KEY_ESCAPE)) {
		cout << "Esc" << endl;
		glfwSetWindowShouldClose(kernel->window.get(), true);
#ifdef __EMSCRIPTEN__
		emscripten_cancel_main_loop();
#endif
		return;
	}

	kernel->world->update(elapsed_time);

	glfwSwapBuffers(kernel->window.get());

	kernel->last_time = current_time;
}

void kernel_destroy(EngineKernel* kernel)
{
	cout << "System shutting down..." << endl;

	kernel->world.reset();

	kernel->renderer->destroy_all_images();

	kernel->renderer.reset();

	kernel->window.reset();

	glfwTerminate();

	delete kernel;
}