#pragma once

#include <glm/glm.hpp>

#include <vector>

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

		auto create_box(float mass, const glm::mat4& model_matrix)->Collidable*;
		auto create_sphere(float const size, float mass, const glm::mat4& model_matrix)->Collidable*;
		auto create_capsule(float mass, const glm::mat4& model_matrix)->Collidable*;
		auto create_mesh(float mass, const std::vector<float>& vertices, const std::vector<unsigned int>& indices, const glm::mat4& model_matrix) -> Collidable*;
		
		auto destroy_collidable(Collidable* collidable)->void;
		
		auto apply_force(Collidable* collidable, glm::vec3 force)->void;
		auto raycast(glm::vec3 from, glm::vec3 to)->bool;

		auto get_model_matrix(Collidable* collidable, glm::mat4& model_matrix)->void;
		
		auto step(float dt, int max_steps)->void;
	private:
		btDefaultCollisionConfiguration* collision_cfg;
		btBroadphaseInterface* broadphase;
		btDiscreteDynamicsWorld* dynamics_world;
	};
}
