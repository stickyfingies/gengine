#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "assets.h"
#include "camera.hpp"
#include "gpu.h"
#include "physics.h"
#include "world.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "glm/gtc/matrix_transform.hpp"
#include <glm/ext/matrix_transform.hpp>

using namespace std;

namespace gengine {

class WasmWorld : public World {

	GLFWwindow* window;
	shared_ptr<gpu::RenderDevice> renderer;
	unique_ptr<PhysicsEngine> physics_engine;
	Camera camera;
	TextureFactory texture_factory;
	gengine::SceneAsset scene;
	gpu::ShaderPipeline* pipeline;
	vector<gpu::Geometry*> meshes;
	vector<gpu::Descriptors*> descriptors;
	vector<glm::mat4> matrices;

public:
	WasmWorld(GLFWwindow* window, shared_ptr<gpu::RenderDevice> renderer)
		: window{window}, renderer{renderer}
	{
		cout << "Hello, Web!" << endl;

		physics_engine = make_unique<PhysicsEngine>();

		camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f));

		const auto vert = gengine::load_file("./data/opengl/basic.vert.glsl");
		const auto frag = gengine::load_file("./data/opengl/basic.frag.glsl");
		pipeline = renderer->create_pipeline(vert, frag);

		// pipeline = renderer->create_pipeline(
		// 	gengine::load_file("./data/cube.vert.spv"), gengine::load_file("./data/cube.frag.spv"));

		// Stack
		const float vertices[] = {
			0.5f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f, -0.5f, 0.5f, 0.0f};
		const float vertices_aux[] = {0.f, 0.f, 0.f, 1.0f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.0f,
									  0.f, 0.f, 0.f, 0.0f, 0.0f, 0.f, 0.f, 0.f, 0.0f, 1.0f};
		const unsigned int indices[] = {0, 1, 3, 1, 2, 3};

		/// Stack --> Heap
		gengine::GeometryAsset geometry{};
		geometry.vertices.assign(vertices, vertices + sizeof(vertices) / sizeof(*vertices));
		geometry.indices.assign(indices, indices + sizeof(indices) / sizeof(*indices));
		geometry.vertices_aux.assign(
			vertices_aux, vertices_aux + sizeof(vertices_aux) / sizeof(*vertices_aux));

		// Game object
		const auto triangle =
			renderer->create_geometry(geometry.vertices, geometry.vertices_aux, geometry.indices);
		meshes.push_back(triangle);
		auto matrix = glm::mat4(1.0f);
		matrix = glm::translate(matrix, glm::vec3(0.f, 0.f, -1.f));
		matrices.push_back(matrix);
		const auto albedo = texture_factory.load_image_from_file("./data/Albedo.png");
		const auto albedo_texture = renderer->create_image(
			albedo->name, albedo->width, albedo->height, albedo->channel_count, albedo->data);
		const auto descriptor =
			renderer->create_descriptors(pipeline, albedo_texture, glm::vec3(1.f, 1.f, 1.f));
		descriptors.push_back(descriptor);
	}

	~WasmWorld() { renderer->destroy_pipeline(pipeline); }

	void update(double elapsed_time) override
	{
		renderer->render(camera.get_view_matrix(), pipeline, matrices, meshes, descriptors, [] {});

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