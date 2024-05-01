#include <GLFW/glfw3.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "assets.h"
#include "camera.hpp"
#include "physics.h"
#include "renderer/renderer.h"

#include <iostream>
#include <vector>

struct RenderComponent {
	gengine::Buffer* vbo;
	gengine::Buffer* ebo;
	unsigned long index_count;
};

struct WindowData {
	double mouse_x;
	double mouse_y;

	double delta_mouse_x;
	double delta_mouse_y;
};

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

	camera.Position = glm::vec3(transforms[0][3]);
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

auto update_renderer(
	Camera& camera,
	gengine::RenderDevice* renderer,
	gengine::ShaderPipeline* pipeline,
	const std::vector<RenderComponent>& render_components,
	const std::vector<glm::mat4>& transforms) -> void
{
	const auto view = camera.get_view_matrix();

	const auto ctx = renderer->alloc_context();
	if (!ctx) {
		return;
	}
	ctx->begin();
	ctx->bind_pipeline(pipeline);
	for (auto i = 0; i < transforms.size(); ++i) {
		ctx->push_constants(pipeline, transforms[i], glm::value_ptr(view));
		ctx->bind_geometry_buffers(render_components[i].vbo, render_components[i].ebo);

		ctx->draw(render_components[i].index_count, 1);
	}
	ctx->end();

	renderer->execute_context(ctx);
	renderer->free_context(ctx);
}

auto create_game_object(
	std::string_view path,
	gengine::RenderDevice* renderer,
	gengine::PhysicsEngine& physics_engine,
	std::vector<RenderComponent>& render_components,
	std::vector<gengine::Collidable*>& collidables,
	std::vector<glm::mat4>& transforms,
	bool makeMesh = false) -> void
{
	const auto geometries = gengine::load_vertex_buffer(path);
	for (const auto& geom : geometries) {
		const auto& [t, vertices, vertices_aux, indices] = geom;

		// TODO: parameterize this separately
		const auto flip_y_coords = makeMesh;

		auto gpu_data = std::vector<float>{};
		for (int i = 0; i < vertices.size() / 3; i++) {
			const auto v = (i * 3);
			gpu_data.push_back(vertices[v + 0]);
			gpu_data.push_back((flip_y_coords ? -1 : 1) * vertices[v + 1]);
			gpu_data.push_back(vertices[v + 2]);
			const auto a = (i * 5);
			gpu_data.push_back(vertices_aux[a + 0]);
			gpu_data.push_back(vertices_aux[a + 1]);
			gpu_data.push_back(vertices_aux[a + 2]);
			gpu_data.push_back(vertices_aux[a + 3]);
			gpu_data.push_back(vertices_aux[a + 4]);
		}

		const auto vbo = renderer->create_buffer(
			{gengine::BufferInfo::Usage::VERTEX, sizeof(float), gpu_data.size()}, gpu_data.data());
		const auto ebo = renderer->create_buffer(
			{gengine::BufferInfo::Usage::INDEX, sizeof(unsigned int), indices.size()},
			indices.data());

		render_components.push_back({vbo, ebo, indices.size()});

		// Generate a physics model when makeMesh == true
		if (makeMesh) {
			auto tr = glm::mat4(1.0f);
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

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// system startup

	auto physics_engine = gengine::PhysicsEngine();

	const auto window = glfwCreateWindow(1280, 720, "Hello World", nullptr, nullptr);

	auto window_data = WindowData{};

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetWindowUserPointer(window, &window_data);

	auto camera = Camera(glm::vec3(0.0f, 5.0f, 90.0f));

	const auto renderer = gengine::RenderDevice::create(window);

	// load game data

	auto transforms = std::vector<glm::mat4>{};
	auto collidables = std::vector<gengine::Collidable*>{};
	auto render_components = std::vector<RenderComponent>{};

	// const auto map_geometries = gengine::load_vertex_buffer("../../data/map.obj");
	// const auto spinny_geometries = gengine::load_vertex_buffer("../../data/spinny.obj");

	// const auto& [map_t, map_vertices, _, map_indices] = map_geometries[0];
	// const auto& [spinny_t, spinny_vertices, __, spinny_indices] = spinny_geometries[0];

	const auto texture = gengine::load_image("./data/albedo.png");

	// create game resources

	// const auto map_vbo = renderer->create_buffer(
	// 	{gengine::BufferInfo::Usage::VERTEX, sizeof(float), map_vertices.size()},
	// map_vertices.data()); const auto map_ebo = renderer->create_buffer(
	// 	{gengine::BufferInfo::Usage::INDEX, sizeof(unsigned int), map_indices.size()},
	// map_indices.data());

	// const auto spinny_vbo = renderer->create_buffer(
	// 	{gengine::BufferInfo::Usage::VERTEX, sizeof(float), spinny_vertices.size()},
	// spinny_vertices.data()); const auto spinny_ebo = renderer->create_buffer(
	// 	{gengine::BufferInfo::Usage::INDEX, sizeof(unsigned int), spinny_indices.size()},
	// spinny_indices.data());

	const auto albedo = renderer->create_image(
		{texture.width, texture.height, texture.channel_count}, texture.data);

	const auto pipeline = renderer->create_pipeline(
		gengine::load_file("./data/cube.vert.spv"),
		gengine::load_file("./data/cube.frag.spv"),
		albedo);

	gengine::unload_image(texture);

	// render_components.push_back({spinny_vbo, spinny_ebo, spinny_indices.size()}); // player
	// render_components.push_back({spinny_vbo, spinny_ebo, spinny_indices.size()});
	// render_components.push_back({map_vbo, map_ebo, map_indices.size()});

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
	{
		// const auto mass = 0.0f;

		// auto transform = glm::mat4(1.0f);

		// transforms.push_back(transform);
		// collidables.push_back(physics_engine.create_mesh(mass, map_vertices, map_indices,
		// transform));
	}

	create_game_object(
		"./data/spinny.obj", renderer, physics_engine, render_components, collidables, transforms);
	create_game_object(
		"./data/spinny.obj", renderer, physics_engine, render_components, collidables, transforms);
	create_game_object(
		"./data/map.obj",
		renderer,
		physics_engine,
		render_components,
		collidables,
		transforms,
		true);

	// core game loop

	auto last_displayed_fps = glfwGetTime();
	auto frame_count = 0u;

	auto last_time = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {
		++frame_count;

		const auto current_time = glfwGetTime();
		const auto elapsed_time = current_time - last_time;

		// log average fps to console
		if (glfwGetTime() - last_displayed_fps >= 1.0) {
			std::cout << "[dbg ]\t ms/frame: " << 1000.0 / frame_count << std::endl;
			std::cout << transforms.size() << ", " << collidables.size() << ", "
					  << render_components.size() << std::endl;
			last_displayed_fps = glfwGetTime();
			frame_count = 0;
		}

		update_physics(camera, physics_engine, collidables, transforms, elapsed_time);

		glfwPollEvents();

		update_input(window, camera, physics_engine, collidables[0], transforms, elapsed_time);

		update_renderer(camera, renderer, pipeline, render_components, transforms);

		glfwSwapBuffers(window);

		last_time = current_time;
	}

	// unload game data

	for (const auto& collidable : collidables) {
		physics_engine.destroy_collidable(collidable);
	}

	// TODO: automate cleanup

	renderer->destroy_pipeline(pipeline);

	renderer->destroy_image(albedo);

	for (auto& renderComponent : render_components) {
		renderer->destroy_buffer(renderComponent.vbo);
		renderer->destroy_buffer(renderComponent.ebo);
	}

	// system shutdown

	gengine::RenderDevice::destroy(renderer);

	glfwDestroyWindow(window);

	std::cout << "[info]\t (module:main) shutdown, terminating window manager" << std::endl;

	glfwTerminate();
}
