#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "world.h"
#include "assets.h"
#include "camera.hpp"
#include "gpu.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>

using namespace std;

namespace gengine {

class WasmWorld : public World {

	shared_ptr<gpu::RenderDevice> renderer;

	GLFWwindow* window;

	TextureFactory texture_factory;

	gengine::SceneAsset scene;

	gpu::ShaderPipeline* pipeline;

	vector<gpu::Geometry*> meshes;

public:
	WasmWorld(GLFWwindow* window, shared_ptr<gpu::RenderDevice> renderer)
		: window{window}, renderer{renderer}
	{
		cout << "Hello, Web!" << endl;

		const auto camera = Camera(glm::vec3(0.0f, 5.0f, 90.0f));
		const auto vert = gengine::load_file("./data/opengl/basic.vert.glsl");
		const auto frag = gengine::load_file("./data/opengl/basic.frag.glsl");
		pipeline = renderer->create_pipeline(vert, frag);

		scene = gengine::load_model(texture_factory, "./data/spinny.obj", false, false);
		for (const auto& geometry : scene.geometries) {
			const auto renderable = renderer->create_geometry(geometry);
			meshes.push_back(renderable);
		}
	}

	~WasmWorld() { 
		// This will never actually happen, because EmScripten doesn't
		// have a "cleanup" phase the way native C++ programs do.
		renderer->destroy_pipeline(pipeline);
	 }

	void update(double elapsed_time) override
	{
		renderer->render({}, pipeline, {}, meshes, {}, [] {});

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

unique_ptr<World> World::create(GLFWwindow* window, shared_ptr<gpu::RenderDevice> renderer)
{
	return make_unique<WasmWorld>(window, renderer);
}

} // namespace gengine