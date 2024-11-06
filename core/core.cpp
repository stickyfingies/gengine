/**
 * This file implements the 'core.h' interface by wrapping OOP classes as functions.
 *
 * In the future, I'd like the different core components to just implement these functions directly.
 */

#include "core.h"
#include "scene.h"

#include <iostream>

glm::mat4* matrix_create() { return new glm::mat4{1.f}; }

void matrix_destroy(glm::mat4* matrix) { delete matrix; }

void matrix_translate(glm::mat4* matrix, float x, float y, float z)
{
	glm::vec3 amount(x, y, z);
	*matrix = glm::translate(*matrix, amount);
}

void matrix_scale(glm::mat4* matrix, float x, float y, float z)
{
	glm::vec3 amount(x, y, z);
	*matrix = glm::scale(*matrix, amount);
}

void model_load(SceneBuilder* sb, char* path, bool flip_uvs, bool flip_tris, bool create_model)
{
	VisualModelSettings settings{flip_uvs, flip_tris, create_model};
	sb->apply_model_settings(path, std::move(settings));
}

void entity_create_capsule(SceneBuilder* sb, glm::mat4* matrix, float mass, const char* path)
{
	sb->add_game_object(*matrix, TactileCapsule{.mass = mass}, VisualModel{.path = path});
}

void entity_create_sphere(
	SceneBuilder* sb, glm::mat4* matrix, float mass, float radius, const char* path)
{
	sb->add_game_object(
		*matrix, TactileSphere{.mass = mass, .radius = radius}, VisualModel{.path = path});
}