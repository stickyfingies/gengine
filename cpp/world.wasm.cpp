#include "world.h"
#include "renderer/renderer.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>

using namespace std;

namespace gengine {

class WasmWorld : public World {

	shared_ptr<RenderDevice> renderer;

	GLFWwindow* window;

public:
	WasmWorld(GLFWwindow* window, shared_ptr<RenderDevice> renderer)
		: window{window}, renderer{renderer}
	{
		cout << "Hello, Web!" << endl;
	}

	~WasmWorld() { cout << "Goodbye, World!" << endl; }

	void update(double elapsed_time) override
	{
		renderer->render({}, nullptr, {}, {}, {}, [] {});

		if (glfwGetKey(window, GLFW_KEY_SPACE)) {
			cout << "Space" << endl;
		}
		if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			cout << "Esc" << endl;
			glfwSetWindowShouldClose(window, true);
#ifdef __EMSCRIPTEN__
			emscripten_cancel_main_loop();
#endif
		}
	}
};

unique_ptr<World> World::create(GLFWwindow* window, shared_ptr<RenderDevice> renderer)
{
	return make_unique<WasmWorld>(window, renderer);
}

} // namespace gengine