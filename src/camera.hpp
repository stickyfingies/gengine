#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

const auto YAW = -90.0f;
const auto PITCH = 0.0f;
const auto SPEED = 2.5f;
const auto SENSITIVITY = 0.1f;
const auto ZOOM = 45.0f;

class Camera {
public:
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;

	float yaw = YAW;
	float pitch = PITCH;

	float movement_speed = SPEED;
	float mouse_sensitivity = SENSITIVITY;
	float zoom = ZOOM;

	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f))
		: Front(glm::vec3(0.0f, 0.0f, -1.0f))
	{
		Position = position;
		WorldUp = up;
		update_camera_vectors();
	}

	auto get_view_matrix() -> glm::mat4 { return glm::lookAt(Position, Position + Front, Up); }

	auto process_mouse_movement(float xoffset, float yoffset, bool constrainpitch = true) -> void
	{
		xoffset *= mouse_sensitivity;
		yoffset *= mouse_sensitivity;

		yaw += xoffset;
		pitch += yoffset;

		if (constrainpitch) {
			// std::clamp doesn't work here for some reason
			if (pitch > 89.0f) {
				pitch = 89.0f;
			}
			if (pitch < -89.0f) {
				pitch = -89.0f;
			}
		}

		update_camera_vectors();
	}

	auto process_mouse_scroll(float yoffset) -> void
	{
		if (zoom >= 1.0f && zoom <= 45.0f) {
			zoom -= yoffset;
		}
		if (zoom <= 1.0f) {
			zoom = 1.0f;
		}
		if (zoom >= 45.0f) {
			zoom = 45.0f;
		}
	}

private:
	auto update_camera_vectors() -> void
	{
		// calculate new front vector

		const auto new_front = glm::vec3{
			cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
			sin(glm::radians(pitch)),
			sin(glm::radians(yaw)) * cos(glm::radians(pitch))};

		Front = glm::normalize(new_front);

		// recalculate right and up vectors

		Right = glm::normalize(glm::cross(Front, WorldUp));
		Up = glm::normalize(glm::cross(Right, Front));
	}
};