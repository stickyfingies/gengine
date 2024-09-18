#include "world.h"
#include "assets.h"
#include "camera.hpp"
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

		const auto camera = Camera(glm::vec3(0.0f, 5.0f, 90.0f));

		// create game resources

		const auto vert = gengine::load_file("./data/cube.vert.glsl");
		const auto frag = gengine::load_file("./data/cube.frag.glsl");
		const auto pipeline = renderer->create_pipeline(vert, frag);
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