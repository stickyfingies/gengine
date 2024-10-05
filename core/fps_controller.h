#pragma once

#include "physics.h"
#include "camera.hpp"

#include <GLFW/glfw3.h>

class FirstPersonController {

	gengine::PhysicsEngine* physics_engine;
	Camera& camera;
	gengine::Collidable* rigidbody;

public:
	FirstPersonController(
		gengine::PhysicsEngine* physics_engine, Camera& camera, gengine::Collidable* rigidbody);

	void update(GLFWwindow* window, float delta);

	void move_forward(float delta);

	void move_backward(float delta);

	void move_left(float delta);

	void move_right(float delta);

	bool jump(float delta);

	bool sprinting = false;
};