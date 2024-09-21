#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "assets.h"
#include "camera.hpp"
#include "gpu.h"
#include "world.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

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

		const float vertices[] = {
			0.5f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f, -0.5f, 0.5f, 0.0f};
		const unsigned int indices[] = {0, 1, 3, 1, 2, 3};

		gengine::GeometryAsset geometry;
		geometry.vertices.assign(vertices, vertices + sizeof(vertices) / sizeof(*vertices));
		geometry.indices.assign(indices, indices + sizeof(indices) / sizeof(*indices));
		const auto triangle = renderer->create_geometry(geometry);
		meshes.push_back(triangle);
	}

	~WasmWorld()
	{
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

			// TODO: abstract this elsewhere
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