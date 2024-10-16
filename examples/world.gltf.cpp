#include "world.h"
#include "camera.hpp"
#include "fps_controller.h"
#include "gpu.h"
#include "physics.h"
#include "scene.h"
#include "window.h"
#ifndef __EMSCRIPTEN__
#include <imgui.h>
#endif
#include <GLFW/glfw3.h>
#include <iostream>

using namespace std;

class NativeWorld : public World {
	// Engine services
	GLFWwindow* window;
	unique_ptr<gengine::PhysicsEngine> physics_engine;
	unique_ptr<Scene> scene;
	shared_ptr<gpu::RenderDevice> gpu;
	gengine::TextureFactory texture_factory{};
	ResourceContainer resources;

	// Game data
	Camera camera;
	unique_ptr<FirstPersonController> fps_controller;
	gpu::ShaderPipeline* pipeline;

public:
	NativeWorld(GLFWwindow* window, shared_ptr<gpu::RenderDevice> gpu) : window{window}, gpu{gpu}
	{
		physics_engine = make_unique<gengine::PhysicsEngine>();

		SceneBuilder sceneBuilder{};

		camera = Camera(glm::vec3(0.0f, 5.0f, 90.0f));

		sceneBuilder.apply_model_settings(
			"./data/skjarisles.glb", {.flip_uvs = false, .flip_triangle_winding = true});

		sceneBuilder.add_game_object(glm::mat4{}, VisualModel{.path = "./data/skjarisles.glb"});

		vector<gpu::VertexAttribute> vertex_attributes;
		vertex_attributes.push_back(gpu::VertexAttribute::VEC3_FLOAT); // position
		vertex_attributes.push_back(gpu::VertexAttribute::VEC3_FLOAT); // normal
		vertex_attributes.push_back(gpu::VertexAttribute::VEC2_FLOAT); // uv

		// TODO: this is incorrect because GL rendering on Desktop Linux will break
#ifdef __EMSCRIPTEN__
		const auto vert = gengine::load_file("./data/gl.vert.glsl");
		const auto frag = gengine::load_file("./data/gl.frag.glsl");
		pipeline = gpu->create_pipeline(vert, frag, vertex_attributes);
#else
		const auto vert = gengine::load_file("./data/vk.vert.spv");
		const auto frag = gengine::load_file("./data/vk.frag.spv");
		pipeline = gpu->create_pipeline(vert, frag, vertex_attributes);
#endif

		scene = sceneBuilder.build(resources, pipeline, gpu.get(), physics_engine.get(), &texture_factory);

		// Assumes all images are uploaded to the GPU and are useless in system memory.
		texture_factory.unload_all_images();

		cout << "[info]\t SUCCESS!! Created scene with " << scene->transforms.size() << " objects"
			 << endl;

		fps_controller =
			make_unique<FirstPersonController>(physics_engine.get(), camera, scene->collidables[0]);

		// start getting things going
		update_physics(0.16f);
	}

	~NativeWorld()
	{
		cout << "~ NativeWorld" << endl;

		for (const auto& rigidbody : resources.rigidbodies) {
			physics_engine->destroy_collidable(rigidbody);
		}

		gpu->destroy_pipeline(pipeline);

		for (auto gpu_geometry : resources.gpu_geometries) {
			gpu->destroy_geometry(gpu_geometry);
		}
	}

	void update(double elapsed_time) override
	{

		update_input(elapsed_time, scene->collidables[0]);
		update_physics(elapsed_time);

		camera.Position = glm::vec3(scene->transforms[0][3]);

#ifndef __EMSCRIPTEN__
		const auto gui_func = [&]() {
			using namespace ImGui;
			// Debug
			SetNextWindowPos({20.0f, 20.0f});
			SetNextWindowSize({0.0f, 0.0f});
			Begin("Debug Menu", nullptr, ImGuiWindowFlags_NoCollapse);
			Text("ms / frame: %.2f", static_cast<float>(elapsed_time));
			Text("Objects: %i", scene->transforms.size());
			// Text("GPU Images: %i", images.size());
			End();
			// Matrices
			SetNextWindowSize({0.0f, 0.0f});
			SetNextWindowPos({200.0f, 20.0f});
			Begin("Matrices");
			PushItemWidth(200.0f);
			for (auto i = 0u; i < scene->transforms.size(); i++) {
				auto& transform = scene->transforms[i];
				const std::string label = "Pos " + i;
				InputFloat3(std::to_string(i).c_str(), &transform[3][0]);
			}
			PopItemWidth();
			End();
			// Textures
			const auto* images_loaded = texture_factory.get_image_log();
			if (images_loaded->size() > 0) {
				SetNextWindowSize({0.0f, 0.0f});
				SetNextWindowPos({500.0f, 20.0f});
				Begin("Texture Loading Timeline", nullptr, ImGuiWindowFlags_NoCollapse);
				for (const auto& image_asset : *images_loaded) {
					Text(
						"%s (%i x %i)",
						image_asset.name.c_str(),
						image_asset.width,
						image_asset.height);
				}
				End();
			}
		};
#else
		const auto gui_func = []() {};
#endif

		gpu->render(
			camera.get_view_matrix(),
			pipeline,
			scene->transforms,
			scene->render_components,
			scene->descriptors,
			gui_func);
	}

	auto update_input(float delta, gengine::Collidable* player) -> void
	{
		auto window_data = static_cast<gengine::WindowData*>(glfwGetWindowUserPointer(window));

		camera.process_mouse_movement(window_data->delta_mouse_x, window_data->delta_mouse_y);

		fps_controller->update(window, delta);

		window_data->delta_mouse_x = 0;
		window_data->delta_mouse_y = 0;
	}

	auto update_physics(float delta) -> void
	{
		physics_engine->step(delta, 10);

		for (auto i = 0; i < scene->collidables.size(); ++i) {
			physics_engine->get_model_matrix(scene->collidables[i], scene->transforms[i]);
		}
	}
};

unique_ptr<World> World::create(GLFWwindow* window, shared_ptr<gpu::RenderDevice> gpu)
{
	return make_unique<NativeWorld>(window, gpu);
}
