#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.hpp"
#include "assets.h"
#include "physics.h"
#include "renderer/renderer.h"

#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <tuple>
#include <vector>

/*
# TODO

- ECS
	This is the biggest issue in main.cpp, as wrangling physics, rendering, and model data is currently very hacky.
	Having a structured way of associating data with a game entity will clean up main.cpp a lot.

- Clean Up Renderer
	renderer.cpp is a fucking mess.  Half of the functionality is baked-in when it should be more flexible, the other
	half is extracted into the public API when it would make things easier for them to be baked in.

- Cry About Physics
	...We'll get to bullet physics later, after the ECS and Render Engine are all cleaned up.  I really don't want to
	deal with bullet at the moment.
*/

namespace
{
	Camera camera(glm::vec3(0.0f, 5.0f, 90.0f));

	auto last_mouse_x = 0.0f;
	auto last_mouse_y = 0.0f;

	std::vector<glm::mat4> transforms;
	std::vector<gengine::Collidable*> collidables;
	std::vector<gengine::RenderComponent> render_components;

	auto mouse_callback(GLFWwindow* window, double pos_x, double pos_y)->void
	{
		const auto offset_x = pos_x - last_mouse_x;
		const auto offset_y = last_mouse_y - pos_y;

		last_mouse_x = pos_x;
		last_mouse_y = pos_y;

		camera.ProcessMouseMovement(offset_x, offset_y);
	}

	auto load_file(std::string_view path)->std::string
	{
		std::ifstream stream(path.data(), std::ifstream::binary);

		std::stringstream buffer;

		buffer << stream.rdbuf();

		return buffer.str();
	}
}

auto main(int argc, char** argv) -> int
{
	/////
	// Loading
	/////

	const auto [map_vertices, map_indices] = gengine::load_vertex_buffer("../data/map.obj");
	const auto [spinny_vertices, spinny_indices] = gengine::load_vertex_buffer("../data/spinny.obj");

	/////
	// Physics
	/////

	gengine::PhysicsEngine physics_engine;

	{
		const float mass = 0.0f;

		glm::mat4 transform(1.0f);
		// transform = glm::scale(transform, glm::vec3(1000.0f, 1.0f, 1000.0f));

		transforms.push_back(transform);
		collidables.push_back(physics_engine.create_mesh(mass, map_vertices, map_indices, glm::value_ptr(transform)));
	}

	{
		const float mass = 62.0f;

		glm::mat4 transform(1.0f);
		transform = glm::translate(transform, glm::vec3(10.0f, 100.0f, 0.0f));

		transforms.push_back(transform);
		collidables.push_back(physics_engine.create_sphere(1.0f, mass, glm::value_ptr(transform)));
	}

	{
		const float mass = 70.0f;

		glm::mat4 transform(1.0f);
		transform = glm::translate(transform, glm::vec3(20.0f, 100.0f, 20.0f));

		transforms.push_back(transform);
		collidables.push_back(physics_engine.create_capsule(mass, glm::value_ptr(transform)));
	}

	/////
	// Graphics
	/////

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	const auto window = glfwCreateWindow(1280, 720, "Hello World", nullptr, nullptr);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);

	gengine::init_renderer(false);

	const auto renderer = gengine::create_render_device(window);

	const auto map_vbo = renderer->create_buffer({gengine::BufferInfo::Usage::VERTEX, sizeof(float), map_vertices.size()}, map_vertices.data());
	const auto map_ebo = renderer->create_buffer({gengine::BufferInfo::Usage::INDEX, sizeof(unsigned int), map_indices.size()}, map_indices.data());
	const auto spinny_vbo = renderer->create_buffer({gengine::BufferInfo::Usage::VERTEX, sizeof(float), spinny_vertices.size()}, spinny_vertices.data());
	const auto spinny_ebo = renderer->create_buffer({gengine::BufferInfo::Usage::INDEX, sizeof(unsigned int), spinny_indices.size()}, spinny_indices.data());

	render_components.push_back({map_vbo, map_ebo, static_cast<unsigned int>(map_indices.size())});
	render_components.push_back({spinny_vbo, spinny_ebo, static_cast<unsigned int>(spinny_indices.size())});
	render_components.push_back({spinny_vbo, spinny_ebo, static_cast<unsigned int>(spinny_indices.size())});

	const auto pipeline = renderer->create_pipeline(load_file("../data/cube.vert.spv"), load_file("../data/cube.frag.spv"));
	
	while (!glfwWindowShouldClose(window))
	{
		physics_engine.step(1.0f / 1000.0f, 10);

		const auto view = camera.GetViewMatrix();

		for (auto i = 0; i < collidables.size(); ++i)
		{
			physics_engine.get_model_matrix(collidables[i], transforms[i]);
		}

		camera.Position = glm::vec3(transforms[2][3]);

		glfwPollEvents();

		const auto sensitivity = 111.0f;

		if (glfwGetKey(window, GLFW_KEY_ESCAPE))
		{
			glfwSetWindowShouldClose(window, true);
		}

		if (glfwGetKey(window, GLFW_KEY_W))
		{
			physics_engine.apply_force(collidables[2], glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
		}
		else if (glfwGetKey(window, GLFW_KEY_S))
		{
			physics_engine.apply_force(collidables[2], glm::vec3(-camera.Front.x, 0.0f, -camera.Front.z));
		}
		if (glfwGetKey(window, GLFW_KEY_A))
		{
			physics_engine.apply_force(collidables[2], glm::vec3(-camera.Right.x, 0.0f, -camera.Right.z));
		}
		else if (glfwGetKey(window, GLFW_KEY_D))
		{
			physics_engine.apply_force(collidables[2], glm::vec3(camera.Right.x, 0.0f, camera.Right.z));
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE))
		{
			const auto on_ground = physics_engine.raycast(camera.Position, glm::vec3(camera.Position.x, camera.Position.y - 60, camera.Position.z));

			if (on_ground) physics_engine.apply_force(collidables[2], glm::vec3(0.0f, 5.6f, 0.0f));
		}

		if (glfwGetKey(window, GLFW_KEY_LEFT))
		{
			camera.ProcessMouseMovement(-0.25f, 0.0f);
		}
		else if (glfwGetKey(window, GLFW_KEY_RIGHT))
		{
			camera.ProcessMouseMovement(0.25f, 0.0f);
		}
		if (glfwGetKey(window, GLFW_KEY_UP))
		{
			camera.ProcessMouseMovement(0.0f, 0.25f);
		}
		else if (glfwGetKey(window, GLFW_KEY_DOWN))
		{
			camera.ProcessMouseMovement(0.0f, -0.25f);
		}

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

		glfwSwapBuffers(window);
	}

	for (const auto& collidable : collidables)
	{
		physics_engine.destroy_collidable(collidable);
	}

	renderer->destroy_pipeline(pipeline);

	renderer->destroy_buffer(map_vbo);
	renderer->destroy_buffer(map_ebo);
	renderer->destroy_buffer(spinny_vbo);
	renderer->destroy_buffer(spinny_ebo);

	gengine::destroy_render_device(renderer);

	glfwDestroyWindow(window);

	glfwTerminate();
}
