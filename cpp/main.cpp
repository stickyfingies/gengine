#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <GLFW/glfw3.h>

#ifndef __EMSCRIPTEN__
#include "glm/gtc/matrix_transform.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "assets.h"
#include "camera.hpp"
#include "physics.h"

#endif
#include "window.h"
#include "world.h"
#include "renderer/renderer.h"

#include <iostream>
#include <vector>

using namespace gengine;
using namespace std;

namespace {

auto mouse_callback(GLFWwindow* window, double pos_x, double pos_y) -> void
{
	auto window_data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

	window_data->delta_mouse_x = pos_x - window_data->mouse_x;
	window_data->delta_mouse_y = window_data->mouse_y - pos_y;

	window_data->mouse_x = pos_x;
	window_data->mouse_y = pos_y;
}

function<void()> loop;
void main_loop() { loop(); }

} // namespace

auto main(int argc, char** argv) -> int
{
	// Argument processing

	std::cout << "[info]\t Launching " << argv[0] << std::endl;

	bool editor_enabled = false;

	if (argc >= 2 && strcmp(argv[1], "editor") == 0) {
		editor_enabled = true;
		std::cout << "[info]\t EDITOR ENABLED" << std::endl;
	}

	// Engine startup 

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	const auto window = glfwCreateWindow(1280, 720, argv[0], nullptr, nullptr);

	auto window_data = WindowData{};

	if (!editor_enabled) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetCursorPosCallback(window, mouse_callback);
	}

	glfwSetWindowUserPointer(window, &window_data);
	
	// System startup

	shared_ptr<RenderDevice> renderer = RenderDevice::create(window);

	const auto world = World::create(window, renderer);

	// core game loop

	auto last_displayed_fps = glfwGetTime();
	auto frame_count = 0u;

	auto last_time = glfwGetTime();

	float ms_per_frame = 0.0f;

	loop = [&]() {
		++frame_count;

		const auto current_time = glfwGetTime();
		const auto elapsed_time = current_time - last_time;

		// ms/frame
		if (glfwGetTime() - last_displayed_fps >= 1.0) {
			ms_per_frame = 1000.0 / frame_count;
			last_displayed_fps = glfwGetTime();
			frame_count = 0;
		}

		glfwPollEvents();

		world->update(elapsed_time);

		glfwSwapBuffers(window);

		last_time = current_time;
	};

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(main_loop, 0, true);
#else
	while (!glfwWindowShouldClose(window))
		main_loop();
#endif

	// unload game data

	renderer->destroy_all_images();

	// system shutdown

	glfwDestroyWindow(window);

	std::cout << "[info]\t (module:main) shutdown, terminating window manager" << std::endl;

	glfwTerminate();

	std::cout << "Cya" << std::endl;
}