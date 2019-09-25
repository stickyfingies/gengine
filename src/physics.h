#pragma once

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

		auto create_box(float const size[3], float const inertia[3], float mass, float const model_matrix[16])->Collidable*;
		auto destroy_box(Collidable* collidable)->void;
		auto get_model_matrix(Collidable* collidable, float* model_matrix)->void;
		auto step(float dt, int max_steps)->void;
	private:
		btDefaultCollisionConfiguration* collision_cfg;
		btBroadphaseInterface* broadphase;
		btDiscreteDynamicsWorld* dynamics_world;
	};
}