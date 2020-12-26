#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.hpp"
#include "assets.h"
#include "module.h"
#include "physics.h"
#include "renderer/renderer.h"

#include <iostream>
#include <vector>

/*
 * NOTE:	Execution order in this file is REALLY confusing. 
 * 			I'm testing out a new module system that will eventually
 *			work at the scope of the entire file.  For now, main.cpp
 * 			is doing some of it's work in `main`, and some of it inside
 *			the module system.  Seth, if you're reading this later,
 *			and are confused and hating yourself, I *am* sorry.

static RenderDevice* dev;

REGISTER_MODULE("foo")(Registry& reg)
{
	reg.grab(RENDER_DEVICE, &dev);
};

 */

MODULE_CALLBACK("main", START)
{
	std::cout << "[info]\t (module:main) startup, initializing window manager" << std::endl;

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	return true;
};

MODULE_CALLBACK("main", STOP)
{
	std::cout << "[info]\t (module:main) shutdown, terminating window manager" << std::endl;

	glfwTerminate();

	return true;
};

struct RenderComponent
{
	gengine::Buffer* vbo;
	gengine::Buffer* ebo;
	unsigned int index_count;
};

struct WindowData
{
	double mouse_x;
	double mouse_y;

	double delta_mouse_x;
	double delta_mouse_y;
};

namespace
{

auto mouse_callback(GLFWwindow* window, double pos_x, double pos_y)->void
{
	auto window_data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

	window_data->delta_mouse_x = pos_x - window_data->mouse_x;
	window_data->delta_mouse_y = window_data->mouse_y - pos_y;

	window_data->mouse_x = pos_x;
	window_data->mouse_y = pos_y;
}

auto update_physics(Camera& camera, gengine::PhysicsEngine& physics_engine, const std::vector<gengine::Collidable*>& collidables, std::vector<glm::mat4>& transforms)->void
{
	physics_engine.step(1.0f / 1000.0f, 10);

	for (auto i = 0; i < collidables.size(); ++i)
	{
		physics_engine.get_model_matrix(collidables[i], transforms[i]);
	}

	camera.Position = glm::vec3(transforms[2][3]);
}

auto update_input(GLFWwindow* window, Camera& camera, gengine::PhysicsEngine& physics_engine, gengine::Collidable* player, std::vector<glm::mat4>& transforms)->void
{
	auto window_data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

	camera.process_mouse_movement(window_data->delta_mouse_x, window_data->delta_mouse_y);

	window_data->delta_mouse_x = 0;
	window_data->delta_mouse_y = 0;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE))
	{
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE))
	{
		const auto on_ground = physics_engine.raycast(camera.Position, glm::vec3(camera.Position.x, camera.Position.y - 5, camera.Position.z));

		if (on_ground) physics_engine.apply_force(player, glm::vec3(0.0f, 15.0f, 0.0f));
	}

	if (glfwGetKey(window, GLFW_KEY_W))
	{
		physics_engine.apply_force(player, glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
	}
	else if (glfwGetKey(window, GLFW_KEY_S))
	{
		physics_engine.apply_force(player, glm::vec3(-camera.Front.x, 0.0f, -camera.Front.z));
	}
	if (glfwGetKey(window, GLFW_KEY_A))
	{
		physics_engine.apply_force(player, glm::vec3(-camera.Right.x, 0.0f, -camera.Right.z));
	}
	else if (glfwGetKey(window, GLFW_KEY_D))
	{
		physics_engine.apply_force(player, glm::vec3(camera.Right.x, 0.0f, camera.Right.z));
	}
}

auto update_renderer(Camera& camera, gengine::RenderDevice* renderer, gengine::ShaderPipeline* pipeline, const std::vector<RenderComponent>& render_components, const std::vector<glm::mat4>& transforms)->void
{
	const auto view = camera.get_view_matrix();

	const auto ctx = renderer->alloc_context();

	ctx->begin();

	ctx->bind_pipeline(pipeline);

	for (auto i = 0; i < transforms.size(); ++i)
	{
		ctx->push_constants(pipeline, transforms[i], glm::value_ptr(view));
		ctx->bind_geometry_buffers(render_components[i].vbo, render_components[i].ebo);

		ctx->draw(render_components[i].index_count, 1);
	}

	ctx->end();

	renderer->execute_context(ctx);

	renderer->free_context(ctx);
}

}

auto main(int argc, char** argv)->int
{
	std::cout << "[info]\t " << argv[0] << std::endl;

	// system startup

	gengine::execute_module_callbacks(gengine::RuntimeStage::START);

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

	const auto& [map_vertices, map_indices] = gengine::load_vertex_buffer("../../data/map.obj");
	const auto& [spinny_vertices, spinny_indices] = gengine::load_vertex_buffer("../../data/spinny.obj");

	const auto texture = gengine::load_image("../../data/albedo.png");

	const auto map_vbo = renderer->create_buffer({gengine::BufferInfo::Usage::VERTEX, sizeof(float), map_vertices.size()}, map_vertices.data());
	const auto map_ebo = renderer->create_buffer({gengine::BufferInfo::Usage::INDEX, sizeof(unsigned int), map_indices.size()}, map_indices.data());
	
	const auto spinny_vbo = renderer->create_buffer({gengine::BufferInfo::Usage::VERTEX, sizeof(float), spinny_vertices.size()}, spinny_vertices.data());
	const auto spinny_ebo = renderer->create_buffer({gengine::BufferInfo::Usage::INDEX, sizeof(unsigned int), spinny_indices.size()}, spinny_indices.data());

	const auto albedo = renderer->create_image({texture.width, texture.height, texture.channel_count}, texture.data);

	const auto pipeline = renderer->create_pipeline(gengine::load_file("../../data/cube.vert.spv"), gengine::load_file("../../data/cube.frag.spv"), albedo);

	render_components.push_back({map_vbo, map_ebo, static_cast<unsigned int>(map_indices.size())});
	render_components.push_back({spinny_vbo, spinny_ebo, static_cast<unsigned int>(spinny_indices.size())});
	render_components.push_back({spinny_vbo, spinny_ebo, static_cast<unsigned int>(spinny_indices.size())});

	gengine::unload_image(texture);

	{
		const auto mass = 0.0f;

		auto transform = glm::mat4(1.0f);

		transforms.push_back(transform);
		collidables.push_back(physics_engine.create_mesh(mass, map_vertices, map_indices, transform));
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
		const auto mass = 70.0f;

		auto transform = glm::mat4(1.0f);
		
		transform = glm::translate(transform, glm::vec3(20.0f, 100.0f, 20.0f));

		transforms.push_back(transform);
		collidables.push_back(physics_engine.create_capsule(mass, transform));
	}

	// core game loop

	auto last_displayed_fps = glfwGetTime();

	auto frame_count = 0u;

	while (!glfwWindowShouldClose(window))
	{
		++frame_count;

		if (glfwGetTime() - last_displayed_fps >= 1.0)
		{
			std::cout << "[dbg ]\t ms/frame: " << 1000.0 / frame_count << std::endl;
			last_displayed_fps = glfwGetTime();
			frame_count = 0;
		}

		update_physics(camera, physics_engine, collidables, transforms);

		glfwPollEvents();

		update_input(window, camera, physics_engine, collidables[2], transforms);

		update_renderer(camera, renderer, pipeline, render_components, transforms);

		glfwSwapBuffers(window);
	}

	// unload game data

	for (const auto& collidable : collidables)
	{
		physics_engine.destroy_collidable(collidable);
	}

	// TODO: automate cleanup

	renderer->destroy_pipeline(pipeline);

	renderer->destroy_image(albedo);

	renderer->destroy_buffer(map_vbo);
	renderer->destroy_buffer(map_ebo);
	renderer->destroy_buffer(spinny_vbo);
	renderer->destroy_buffer(spinny_ebo);

	// system shutdown

	gengine::RenderDevice::destroy(renderer);

	gengine::execute_module_callbacks(gengine::RuntimeStage::STOP);

	glfwDestroyWindow(window);
}
