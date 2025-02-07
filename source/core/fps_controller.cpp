#include "fps_controller.h"

FirstPersonController::FirstPersonController(
	gengine::PhysicsEngine* physics_engine, Camera& camera, gengine::Collidable* rigidbody)
	: physics_engine{physics_engine}, camera{camera}, rigidbody{rigidbody}
{
}

void FirstPersonController::update(GLFWwindow* window, float delta)
{
	sprinting = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);

	if (glfwGetKey(window, GLFW_KEY_SPACE)) {
		jump(delta);
	}

	if (glfwGetKey(window, GLFW_KEY_W)) {
		move_forward(delta);
	}
	else if (glfwGetKey(window, GLFW_KEY_S)) {
		move_backward(delta);
	}
	if (glfwGetKey(window, GLFW_KEY_A)) {
		move_left(delta);
	}
	else if (glfwGetKey(window, GLFW_KEY_D)) {
		move_right(delta);
	}
}

void FirstPersonController::move_forward(float delta)
{
	physics_engine->apply_force(
		rigidbody,
		glm::vec3(camera.Front.x, 0.0f, camera.Front.z) * (delta * 1000) *
			(1.0f + sprinting * 5.0f));
}

void FirstPersonController::move_backward(float delta)
{
	physics_engine->apply_force(
		rigidbody, glm::vec3(-camera.Front.x, 0.0f, -camera.Front.z) * (delta * 1000));
}

void FirstPersonController::move_left(float delta)
{
	physics_engine->apply_force(
		rigidbody, glm::vec3(-camera.Right.x, 0.0f, -camera.Right.z) * (delta * 1000));
}

void FirstPersonController::move_right(float delta)
{
	physics_engine->apply_force(
		rigidbody, glm::vec3(camera.Right.x, 0.0f, camera.Right.z) * (delta * 1000));
}

bool FirstPersonController::jump(float delta)
{
	const bool on_ground = physics_engine->raycast(
		camera.Position, glm::vec3(camera.Position.x, camera.Position.y - 5, camera.Position.z));

	if (on_ground) {
		physics_engine->apply_force(rigidbody, glm::vec3(0.0f, 15.0f, 0.0f) * (delta * 1000));
	}

	return on_ground;
}