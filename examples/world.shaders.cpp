#include "world.h"
#include "assets.h"
#include "gpu.h"
#include "window.h"
#include <GLFW/glfw3.h>
#include <iostream>

using namespace std;

class NativeWorld : public World {
	// Engine services
	GLFWwindow* window;
	shared_ptr<gpu::RenderDevice> gpu;
	gpu::ShaderPipeline* pipeline;

public:
	NativeWorld(GLFWwindow* window, shared_ptr<gpu::RenderDevice> gpu) : window{window}, gpu{gpu}
	{
		// Describe the "shape" of our geometry data
		std::vector<gpu::VertexAttribute> vertex_attributes;
		vertex_attributes.push_back(gpu::VertexAttribute::VEC3_FLOAT); // position

		// TODO: this is incorrect because GL rendering on Desktop Linux will break
#if 1//def __EMSCRIPTEN__
		const auto vert = gengine::load_file("./data/cool.vert.glsl");
		const auto frag = gengine::load_file("./data/cool.frag.glsl");
		pipeline = gpu->create_pipeline(vert, frag, vertex_attributes);
#else
		const auto vert = gengine::load_file("./data/cool.vert.spv");
		const auto frag = gengine::load_file("./data/cool.frag.spv");
		pipeline = gpu->create_pipeline(vert, frag, vertex_attributes);
#endif
	}

	~NativeWorld()
	{
		cout << "~ NativeWorld" << endl;

		gpu->destroy_pipeline(pipeline);
	}

	void update(double elapsed_time) override
	{
		// gpu->render(
		// 	camera.get_view_matrix(),
		// 	pipeline,
		// 	transform,
		// 	geometry,
		// 	descriptor,
		// 	[](){});
	}
};

unique_ptr<World> World::create(GLFWwindow* window, shared_ptr<gpu::RenderDevice> gpu)
{
	return make_unique<NativeWorld>(window, gpu);
}
