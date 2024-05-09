#include "glm/gtc/matrix_transform.hpp"
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "assets.h"
#include "camera.hpp"
#include "physics.h"
#include "renderer/renderer.h"

#include <imgui.h>

#include <iostream>
#include <vector>

struct WindowData {
	double mouse_x;
	double mouse_y;

	double delta_mouse_x;
	double delta_mouse_y;
};

using gengine::Renderable;

namespace {

auto mouse_callback(GLFWwindow* window, double pos_x, double pos_y) -> void
{
	auto window_data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

	window_data->delta_mouse_x = pos_x - window_data->mouse_x;
	window_data->delta_mouse_y = window_data->mouse_y - pos_y;

	window_data->mouse_x = pos_x;
	window_data->mouse_y = pos_y;
}

auto update_physics(
	Camera& camera,
	gengine::PhysicsEngine& physics_engine,
	const std::vector<gengine::Collidable*>& collidables,
	std::vector<glm::mat4>& transforms,
	float delta) -> void
{
	physics_engine.step(delta, 10);

	for (auto i = 0; i < collidables.size(); ++i) {
		physics_engine.get_model_matrix(collidables[i], transforms[i]);
	}
}

auto update_input(
	GLFWwindow* window,
	Camera& camera,
	gengine::PhysicsEngine& physics_engine,
	gengine::Collidable* player,
	std::vector<glm::mat4>& transforms,
	float delta) -> void
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
		const auto on_ground = physics_engine.raycast(
			camera.Position,
			glm::vec3(camera.Position.x, camera.Position.y - 5, camera.Position.z));

		if (on_ground) {
			physics_engine.apply_force(player, glm::vec3(0.0f, 15.0f, 0.0f) * (delta * 1000));
		}
	}

	if (glfwGetKey(window, GLFW_KEY_W)) {
		physics_engine.apply_force(
			player,
			glm::vec3(camera.Front.x, 0.0f, camera.Front.z) * (delta * 1000) *
				(1.0f + sprinting * 5.0f));
	}
	else if (glfwGetKey(window, GLFW_KEY_S)) {
		physics_engine.apply_force(
			player, glm::vec3(-camera.Front.x, 0.0f, -camera.Front.z) * (delta * 1000));
	}
	if (glfwGetKey(window, GLFW_KEY_A)) {
		physics_engine.apply_force(
			player, glm::vec3(-camera.Right.x, 0.0f, -camera.Right.z) * (delta * 1000));
	}
	else if (glfwGetKey(window, GLFW_KEY_D)) {
		physics_engine.apply_force(
			player, glm::vec3(camera.Right.x, 0.0f, camera.Right.z) * (delta * 1000));
	}
}

auto create_game_object(
	std::string_view path,
	gengine::RenderDevice* renderer,
	gengine::PhysicsEngine& physics_engine,
	std::vector<Renderable>& render_components,
	std::vector<gengine::Image*>& images,
	std::vector<gengine::Descriptors*>& descriptors,
	gengine::ShaderPipeline* pipeline,
	std::vector<gengine::Collidable*>& collidables,
	std::vector<glm::mat4>& transforms,
	bool makeMesh = false,
	bool flipUVs = false,
	bool flipWindingOrder = false) -> void
{
	const auto meshes = gengine::load_model(path, flipUVs, flipWindingOrder);
	for (const auto& mesh : meshes) {
		const auto& [t, vertices, vertices_aux, indices, textures, color] = mesh;

		const auto renderable = renderer->create_renderable(vertices, vertices_aux, indices);

		render_components.push_back(renderable);

		// Materials

		gengine::ImageAsset texture_0{};
		if (textures.size() > 0) {
			texture_0 = textures[0];
		}
		if (texture_0.width == 0) {
			texture_0 = *gengine::load_image_from_file("./data/solid_white.png");
		}

		auto albedo = renderer->create_image(texture_0);

		images.push_back(albedo);

		const auto descriptor_0 = renderer->create_descriptors(pipeline, albedo, color);

		descriptors.push_back(descriptor_0);

		// gengine::unload_image(texture_0);

		// Generate a physics model when makeMesh == true
		if (makeMesh) {
			// auto tr = glm::mat4{1.0f};
			auto tr = t;
			// tr = glm::rotate(tr, 3.14f, glm::vec3(0, 1, 0));
			transforms.push_back(tr);
			collidables.push_back(physics_engine.create_mesh(0.0f, vertices, indices, tr));
		}
	}
}

} // namespace

auto main(int argc, char** argv) -> int
{
	std::cout << "[info]\t " << argv[0]
			  << "[info]\t (module:main) startup, initializing window manager" << std::endl;

	bool editor_enabled = false;

	if (argc >= 2 && strcmp(argv[1], "editor") == 0) {
		editor_enabled = true;
		std::cout << "[info]\t EDITOR ENABLED" << std::endl;
	}

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// system startup

	auto physics_engine = gengine::PhysicsEngine();

	const auto window = glfwCreateWindow(1280, 720, "Hello World", nullptr, nullptr);

	auto window_data = WindowData{};

	if (!editor_enabled) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetCursorPosCallback(window, mouse_callback);
	}

	glfwSetWindowUserPointer(window, &window_data);

	auto camera = Camera(glm::vec3(0.0f, 5.0f, 90.0f));

	const auto renderer = gengine::RenderDevice::create(window);

	// load game data

	auto transforms = std::vector<glm::mat4>{};
	auto collidables = std::vector<gengine::Collidable*>{};
	auto render_components = std::vector<Renderable>{};
	auto images = std::vector<gengine::Image*>();
	auto descriptors = std::vector<gengine::Descriptors*>{};

	// create game resources

	const auto pipeline = renderer->create_pipeline(
		gengine::load_file("./data/cube.vert.spv"), gengine::load_file("./data/cube.frag.spv"));

	// Create physics bodies
	{ // player
		const auto mass = 70.0f;

		auto transform = glm::mat4(1.0f);

		transform = glm::translate(transform, glm::vec3(20.0f, 100.0f, 20.0f));

		transforms.push_back(transform);
		collidables.push_back(physics_engine.create_capsule(mass, transform));
	}
	{
		const auto mass = 62.0f;

		auto transform = glm::mat4(1.0f);

		transform = glm::translate(transform, glm::vec3(10.0f, 100.0f, 0.0f));
		transform = glm::scale(transform, glm::vec3(6.0f, 6.0f, 6.0f));

		transforms.push_back(transform);
		collidables.push_back(physics_engine.create_sphere(1.0f, mass, transform));
	}

	create_game_object(
		"./data/spinny.obj",
		renderer,
		physics_engine,
		render_components,
		images,
		descriptors,
		pipeline,
		collidables,
		transforms,
		false,
		false,
		false);
	create_game_object(
		"./data/spinny.obj",
		renderer,
		physics_engine,
		render_components,
		images,
		descriptors,
		pipeline,
		collidables,
		transforms,
		false,
		false,
		false);
	create_game_object(
		"./data/skjar-isles/skjarisles.glb",
		renderer,
		physics_engine,
		render_components,
		images,
		descriptors,
		pipeline,
		collidables,
		transforms,
		true,
		true,
		false);

	// core game loop

	auto last_displayed_fps = glfwGetTime();
	auto frame_count = 0u;

	auto last_time = glfwGetTime();

	std::cout << "[info]\t SUCCESS!! Created scene with " << transforms.size() << " objects"
			  << std::endl;

	float ms_per_frame = 0.0f;

	update_physics(camera, physics_engine, collidables, transforms, 0.16f);

	while (!glfwWindowShouldClose(window)) {
		++frame_count;

		const auto current_time = glfwGetTime();
		const auto elapsed_time = current_time - last_time;

		// log average fps to console
		if (glfwGetTime() - last_displayed_fps >= 1.0) {
			ms_per_frame = 1000.0 / frame_count;
			std::cout << "[dbg ]\t ms/frame: " << ms_per_frame << std::endl;
			last_displayed_fps = glfwGetTime();
			frame_count = 0;
		}
		if (!editor_enabled) {
			update_physics(camera, physics_engine, collidables, transforms, elapsed_time);
		}

		camera.Position = glm::vec3(transforms[0][3]);

		glfwPollEvents();

		if (!editor_enabled) {
			update_input(window, camera, physics_engine, collidables[0], transforms, elapsed_time);
		}

		renderer->render(
			camera.get_view_matrix(), pipeline, transforms, render_components, descriptors, [&]() {
				using namespace ImGui;
				// Debug
				SetNextWindowPos({20.0f, 20.0f});
				SetNextWindowSize({0.0f, 0.0f});
				Begin("Debug Menu", nullptr, ImGuiWindowFlags_NoCollapse);
				Text("ms / frame: %.2f", ms_per_frame);
				Text("Objects: %i", transforms.size());
				Text("GPU Images: %i", images.size());
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
				const auto* images_loaded = gengine::get_image_log();
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

		glfwSwapBuffers(window);

		last_time = current_time;
	}

	// unload game data

	gengine::unload_all_images();

	for (const auto& collidable : collidables) {
		physics_engine.destroy_collidable(collidable);
	}

	// TODO: automate cleanup

	renderer->destroy_pipeline(pipeline);

	// renderer->destroy_image(albedo);

	for (auto& renderComponent : render_components) {
		renderer->destroy_buffer(renderComponent.vbo);
		renderer->destroy_buffer(renderComponent.ebo);
	}

	for (auto& image : images) {
		renderer->destroy_image(image);
	}

	// system shutdown

	gengine::RenderDevice::destroy(renderer);

	glfwDestroyWindow(window);

	std::cout << "[info]\t (module:main) shutdown, terminating window manager" << std::endl;

	glfwTerminate();
}
