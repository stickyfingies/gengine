#pragma once

#include <glm/glm.hpp>

struct btDefaultCollisionConfiguration;
struct btBroadphaseInterface;
struct btDiscreteDynamicsWorld;

namespace gengine
{
	struct Collidable;

	class PhysicsEngine
	{
	public:
		PhysicsEngine();
		~PhysicsEngine();

		auto create_box(float mass, float const model_matrix[16])->Collidable*;
		auto create_sphere(float const size, float mass, float const model_matrix[16])->Collidable*;
		auto destroy_collidable(Collidable* collidable)->void;
		auto get_model_matrix(Collidable* collidable, glm::mat4& model_matrix)->void;
		auto apply_force(Collidable* collidable, glm::vec3 force)->void;
		auto raycast(glm::vec3 from, glm::vec3 to)->bool;
		auto step(float dt, int max_steps)->void;
	private:
		btDefaultCollisionConfiguration* collision_cfg;
		btBroadphaseInterface* broadphase;
		btDiscreteDynamicsWorld* dynamics_world;
	};
}