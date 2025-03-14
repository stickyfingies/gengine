/**
 * This file represents the public FFI interface for the 'core' module.
 */

#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

struct SceneBuilder;

/**
 * extern "C" to maximize compatibility with various Foreign Function Interfaces.
 */
extern "C" {

glm::mat4* matrix_create();

void matrix_destroy(glm::mat4* matrix);

void matrix_translate(glm::mat4* matrix, float x, float y, float z);

void matrix_scale(glm::mat4* matrix, float x, float y, float z);

SceneBuilder* scene_create();

void scene_destroy(SceneBuilder* sb);

void scene_load_model(SceneBuilder* sb, char* path, bool flip_uvs, bool flip_tris, bool create_model);

void scene_create_capsule(SceneBuilder* sb, glm::mat4* matrix, float mass, const char* path);

void scene_create_sphere(SceneBuilder* sb, glm::mat4* matrix, float mass, float radius, const char* path);
}