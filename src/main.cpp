#include <GLFW/glfw3.h>
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "physics.h"
#include "renderer/renderer.h"

auto main(int argc, char** argv) -> int
{
	/////
	// Physics
	/////

	gengine::PhysicsEngine physics_engine;

	gengine::Collidable* boxA;
	gengine::Collidable* boxB;

	{
		float const size[]{ 20.0f, 20.0f, 20.0f };
		float const inertia[]{ 0.0f, 0.0f, 0.0f };
		float const mass = 0.0f;

		glm::mat4 matrix(1.0f);
		matrix = glm::rotate(matrix, glm::radians(22.0f), glm::vec3(0.0f, -1.25f, -0.5f));
		matrix = glm::translate(matrix, glm::vec3(0.0f, -20.0f, 0.0f));

		boxA = physics_engine.create_box(size, inertia, mass, glm::value_ptr(matrix));
	}

	{
		float const size[]{ 5.0f, 5.0f, 5.0f };
		float const inertia[]{ 1.0f, 1.0f, 1.0f };
		float const mass = 1.0f;

		glm::mat4 matrix(1.0f);
		matrix = glm::translate(matrix, glm::vec3(-10.0f, 50.0f, 0.0f));

		boxB = physics_engine.create_box(size, inertia, mass, glm::value_ptr(matrix));
	}

	/////
	// Graphics
	/////

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	auto window = glfwCreateWindow(1280, 720, "Hello World", nullptr, nullptr);

	gengine::Renderer renderer_vk(window);

	while (!glfwWindowShouldClose(window))
	{
		physics_engine.step(1.0f / 1000.0f, 10);

		renderer_vk.start_frame();

		glm::mat4 model;

		physics_engine.get_model_matrix(boxA, glm::value_ptr(model));
		model = glm::scale(model, glm::vec3(40.0f, 40.0f, 40.0f));
		renderer_vk.draw_box(glm::value_ptr(model));

		physics_engine.get_model_matrix(boxB, glm::value_ptr(model));
		model = glm::scale(model, glm::vec3(10.0f, 10.0f, 10.0f));
		renderer_vk.draw_box(glm::value_ptr(model));

		renderer_vk.end_frame();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	physics_engine.destroy_box(boxA);
	physics_engine.destroy_box(boxB);

	glfwDestroyWindow(window);

	glfwTerminate();
}