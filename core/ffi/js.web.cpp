/**
 * This file is a C++ game script which uses Embind to call JS scripts.
 *
 * It is used on WEB platforms with JS scripts.
 */

#ifndef __EMSCRIPTEN__
#error Emscripten is required to build (js.web.cpp).
#endif

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "camera.hpp"
#include "core.h"
#include "fps_controller.h"
#include "gpu.h"
#include "physics.h"
#include "scene.h"
#include "window.h"
#include "world.h"

#include <GLFW/glfw3.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

namespace em = emscripten;
using namespace std;

/**
 * JS wrapper for glm::mat4
 */
struct JSTransform {
	glm::mat4 matrix;
	JSTransform() { matrix = glm::mat4(1.0f); }

	JSTransform* translate(float x, float y, float z)
	{
		glm::vec3 amount(x, y, z);
		matrix = glm::translate(matrix, amount);
		return this;
	}

	JSTransform* scale(float x, float y, float z)
	{
		glm::vec3 amount(x, y, z);
		matrix = glm::scale(matrix, amount);
		return this;
	}
};

/**
 * JS function for printing a string to stdout
 */
void js_print(const char* text) { cout << "[JS]: " << text << endl; }

/**
 * JS wrapper around SceneBuilder
 */
void js_load_model(SceneBuilder* sb, std::string path, VisualModelSettings* settings)
{
	sb->apply_model_settings(path, std::move(*settings));
}

/**
 * JS wrapper around SceneBuilder
 */
void js_create_capsule(SceneBuilder* sb, JSTransform* transform, float mass, const char* path)
{
	sb->add_game_object(transform->matrix, TactileCapsule{.mass = mass}, VisualModel{.path = path});
}

/**
 * JS wrapper around SceneBuilder
 */
void js_create_sphere(
	SceneBuilder* sb, JSTransform* transform, float mass, float radius, const char* path)
{
	sb->add_game_object(
		transform->matrix,
		TactileSphere{.mass = mass, .radius = radius},
		VisualModel{.path = path});
}

class JsSceneBuilder {
private:
	SceneBuilder* sb;

public:
	JsSceneBuilder(SceneBuilder* sb) : sb{sb} {}

	void applyModelSettings(string path, VisualModelSettings s)
	{
		sb->apply_model_settings(path, std::move(s));
	}

	void createCapsule(JSTransform& t, float mass, string path)
	{
		sb->add_game_object(t.matrix, TactileCapsule{.mass = mass}, VisualModel{.path = path});
	}

	void createSphere(JSTransform& t, float mass, float radius, string path)
	{
		sb->add_game_object(
			t.matrix, TactileSphere{.mass = mass, .radius = radius}, VisualModel{.path = path});
	}
};

EMSCRIPTEN_BINDINGS(core)
{
	em::class_<JsSceneBuilder>("SceneBuilder")
		.function("applyModelSettings", &JsSceneBuilder::applyModelSettings)
		.function("createCapsule", &JsSceneBuilder::createCapsule)
		.function("createSphere", &JsSceneBuilder::createSphere);

	em::class_<JSTransform>("Transform")
		.constructor<>()
		.function("translate", &JSTransform::translate, em::return_value_policy::reference())
		.function("scale", &JSTransform::scale, em::return_value_policy::reference());

	em::class_<VisualModelSettings>("VisualModelSettings").constructor<bool, bool, bool>();
}

/**
 * The actual C++ script which binds to JS
 */
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

		JsSceneBuilder jsSceneBuilder(&sceneBuilder);
		uintptr_t sceneBuilderPtr = reinterpret_cast<uintptr_t>(&sceneBuilder);
		em::val js_create = em::val::global("create");
		if (js_create.as<bool>()) {
			js_create(jsSceneBuilder);
		}
		else {
			cerr << "This app uses JS scripts, but the 'create' function was not defined." << endl;
		}

		camera = Camera(glm::vec3(0.0f, 5.0f, 90.0f));

		// auto ball_pos = glm::mat4(1.0f);
		// ball_pos = glm::translate(ball_pos, glm::vec3(10.0f, 100.0f, 0.0f));
		// ball_pos = glm::scale(ball_pos, glm::vec3(6.0f, 6.0f, 6.0f));

		// sceneBuilder.add_game_object(
		// 	ball_pos,
		// 	TactileSphere{.mass = 62.0f, .radius = 1.0f},
		// 	VisualModel{.path = "./data/spinny.obj"});

		sceneBuilder.add_game_object(glm::mat4{}, VisualModel{.path = "./data/map.obj"});

		// Describe the "shape" of our geometry data
		std::vector<gpu::VertexAttribute> vertex_attributes;
		vertex_attributes.push_back(gpu::VertexAttribute::VEC3_FLOAT); // position
		vertex_attributes.push_back(gpu::VertexAttribute::VEC3_FLOAT); // normal
		vertex_attributes.push_back(gpu::VertexAttribute::VEC2_FLOAT); // uv

		// TODO: this is incorrect because GL rendering on Desktop Linux will break
#ifdef __EMSCRIPTEN__
		const auto vert = gengine::load_file("./data/gl.vert.glsl");
		const auto frag = gengine::load_file("./data/gl.frag.glsl");
		pipeline = gpu->create_pipeline(vert, frag, vertex_attributes);
#else
		const auto vert = gengine::load_file("./data/cube.vert.spv");
		const auto frag = gengine::load_file("./data/cube.frag.spv");
		pipeline = gpu->create_pipeline(vert, frag, vertex_attributes);
#endif

		scene = sceneBuilder.build(
			resources, pipeline, gpu.get(), physics_engine.get(), &texture_factory);

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

		const auto gui_func = []() {};

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
