#include "world.h"
#include "camera.hpp"
#include "physics.h"
#include "renderer/renderer.h"
#include "window.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>

namespace gengine {

using namespace std;

class NativeWorld : public World {
	// Engine services
	GLFWwindow* window;
	unique_ptr<PhysicsEngine> physics_engine;
	shared_ptr<RenderDevice> renderer;
	TextureFactory texture_factory{};

	// Game data
	Camera camera;
	vector<glm::mat4> transforms{};
	vector<Collidable*> collidables{};
	vector<Renderable> render_components{};
	vector<Descriptors*> descriptors{};
	ShaderPipeline* pipeline;

public:
	NativeWorld(GLFWwindow* window, shared_ptr<RenderDevice> renderer)
		: window{window}, renderer{renderer}
	{
		physics_engine = make_unique<PhysicsEngine>();

		camera = Camera(glm::vec3(0.0f, 5.0f, 90.0f));

		// create game resources

		pipeline = renderer->create_pipeline(
			gengine::load_file("./data/cube.vert.spv"), gengine::load_file("./data/cube.frag.spv"));

		// Create physics bodies
		{ // player
			const auto mass = 70.0f;

			auto transform = glm::mat4(1.0f);

			transform = glm::translate(transform, glm::vec3(20.0f, 100.0f, 20.0f));

			transforms.push_back(transform);
			collidables.push_back(physics_engine->create_capsule(mass, transform));
		}
		{
			const auto mass = 62.0f;

			auto transform = glm::mat4(1.0f);

			transform = glm::translate(transform, glm::vec3(10.0f, 100.0f, 0.0f));
			transform = glm::scale(transform, glm::vec3(6.0f, 6.0f, 6.0f));

			transforms.push_back(transform);
			collidables.push_back(physics_engine->create_sphere(1.0f, mass, transform));
		}

		create_game_object(
			"./data/spinny.obj",
			render_components,
			descriptors,
			pipeline,
			collidables,
			transforms,
			false,
			false,
			false);
		create_game_object(
			"./data/spinny.obj",
			render_components,
			descriptors,
			pipeline,
			collidables,
			transforms,
			false,
			false,
			false);
		create_game_object(
			"./data/skjar-isles/skjarisles.glb",
			render_components,
			descriptors,
			pipeline,
			collidables,
			transforms,
			true,
			true,
			false);

		// Assumes all images are uploaded to the GPU and are useless in system memory.
		texture_factory.unload_all_images();

		cout << "[info]\t SUCCESS!! Created scene with " << transforms.size() << " objects" << endl;

		// start getting things going
		update_physics(0.16f);
	}

	~NativeWorld()
	{
		for (const auto& collidable : collidables) {
			physics_engine->destroy_collidable(collidable);
		}

		// TODO: automate cleanup

		renderer->destroy_pipeline(pipeline);

		// renderer->destroy_image(albedo);

		for (auto& renderComponent : render_components) {
			renderer->destroy_buffer(renderComponent.vbo);
			renderer->destroy_buffer(renderComponent.ebo);
		}
	}

	void update(double elapsed_time) override
	{
		// TODO get this from main.cpp
		const bool editor_enabled = false;

		if (!editor_enabled) {
			update_input(elapsed_time, collidables[0]);
			update_physics(elapsed_time);
		}

		camera.Position = glm::vec3(transforms[0][3]);

		renderer->render(
			camera.get_view_matrix(), pipeline, transforms, render_components, descriptors, [&]() {
				using namespace ImGui;
				// Debug
				SetNextWindowPos({20.0f, 20.0f});
				SetNextWindowSize({0.0f, 0.0f});
				Begin("Debug Menu", nullptr, ImGuiWindowFlags_NoCollapse);
				Text("ms / frame: %.2f", static_cast<float>(elapsed_time));
				Text("Objects: %i", transforms.size());
				// Text("GPU Images: %i", images.size());
				End();
				// Matrices
				SetNextWindowSize({0.0f, 0.0f});
				SetNextWindowPos({200.0f, 20.0f});
				Begin("Matrices");
				PushItemWidth(200.0f);
				for (auto i = 0u; i < transforms.size(); i++) {
					auto& transform = transforms[i];
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
			});
	}

	auto update_input(float delta, Collidable* player) -> void
	{
		auto sprinting = false;

		auto window_data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

		camera.process_mouse_movement(window_data->delta_mouse_x, window_data->delta_mouse_y);

		window_data->delta_mouse_x = 0;
		window_data->delta_mouse_y = 0;

		if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, true);
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
			sprinting = true;
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE)) {
			const auto on_ground = physics_engine->raycast(
				camera.Position,
				glm::vec3(camera.Position.x, camera.Position.y - 5, camera.Position.z));

			if (on_ground) {
				physics_engine->apply_force(player, glm::vec3(0.0f, 15.0f, 0.0f) * (delta * 1000));
			}
		}

		if (glfwGetKey(window, GLFW_KEY_W)) {
			physics_engine->apply_force(
				player,
				glm::vec3(camera.Front.x, 0.0f, camera.Front.z) * (delta * 1000) *
					(1.0f + sprinting * 5.0f));
		}
		else if (glfwGetKey(window, GLFW_KEY_S)) {
			physics_engine->apply_force(
				player, glm::vec3(-camera.Front.x, 0.0f, -camera.Front.z) * (delta * 1000));
		}
		if (glfwGetKey(window, GLFW_KEY_A)) {
			physics_engine->apply_force(
				player, glm::vec3(-camera.Right.x, 0.0f, -camera.Right.z) * (delta * 1000));
		}
		else if (glfwGetKey(window, GLFW_KEY_D)) {
			physics_engine->apply_force(
				player, glm::vec3(camera.Right.x, 0.0f, camera.Right.z) * (delta * 1000));
		}
	}

	auto update_physics(float delta) -> void
	{
		physics_engine->step(delta, 10);

		for (auto i = 0; i < collidables.size(); ++i) {
			physics_engine->get_model_matrix(collidables[i], transforms[i]);
		}
	}

	auto create_game_object(
		std::string_view path,
		std::vector<Renderable>& render_components,
		std::vector<gengine::Descriptors*>& descriptors,
		gengine::ShaderPipeline* pipeline,
		std::vector<gengine::Collidable*>& collidables,
		std::vector<glm::mat4>& transforms,
		bool makeMesh = false,
		bool flipUVs = false,
		bool flipWindingOrder = false) -> void
	{
		std::vector<gengine::Descriptors*> descriptor_cache;
		std::vector<gengine::Renderable> renderable_cache;

		const auto scene = gengine::load_model(texture_factory, path, flipUVs, flipWindingOrder);

		/// Material --> Descriptors
		for (const auto& material : scene.materials) {

			// TODO - move this inside assets.cpp
			gengine::ImageAsset texture_0{};
			if (material.textures.size() > 0) {
				texture_0 = material.textures[0];
			}
			if (texture_0.width == 0) {
				texture_0 = *texture_factory.load_image_from_file("./data/solid_white.png");
			}

			auto albedo = renderer->create_image(texture_0);

			const auto descriptor_0 =
				renderer->create_descriptors(pipeline, albedo, material.color);

			descriptor_cache.push_back(descriptor_0);
		}

		/// Geometry --> Renderable
		for (const auto& geometry : scene.geometries) {
			const auto renderable = renderer->create_renderable(geometry);
			renderable_cache.push_back(renderable);
		}

		// Game objects
		for (const auto& object : scene.objects) {
			const auto& [t, geometry_idx, material_idx] = object;

			std::cout << "Creating object: " << geometry_idx << ", " << material_idx << " | "
					  << renderable_cache.size() << ", " << descriptor_cache.size() << std::endl;

			render_components.push_back(renderable_cache[geometry_idx]);

			// Materials

			descriptors.push_back(descriptor_cache[material_idx]);

			// gengine::unload_image(texture_0);

			// Generate a physics model when makeMesh == true
			if (makeMesh) {
				// auto tr = glm::mat4{1.0f};
				auto tr = t;
				// tr = glm::rotate(tr, 3.14f, glm::vec3(0, 1, 0));
				transforms.push_back(tr);
				collidables.push_back(
					physics_engine->create_mesh(0.0f, scene.geometries[geometry_idx], tr));
			}
		}
	}
};

unique_ptr<World> World::create(GLFWwindow* window, shared_ptr<RenderDevice> renderer)
{
	return make_unique<NativeWorld>(window, renderer);
}

} // namespace gengine