#include "physics.h"

#include <btBulletDynamicsCommon.h>

namespace gengine
{
	struct Collidable
	{
		btMotionState* motion_state;
		btCollisionShape* shape;
		btRigidBody* body;
	};

	PhysicsEngine::PhysicsEngine()
	{
		collision_cfg = new btDefaultCollisionConfiguration();

		broadphase = new btAxisSweep3(btVector3(-100, -100, -100), btVector3(100, 100, 100), 128);

		dynamics_world = new btDiscreteDynamicsWorld(new btCollisionDispatcher(collision_cfg), broadphase, new btSequentialImpulseConstraintSolver(), collision_cfg);

		dynamics_world->setGravity(btVector3(0, -10, 0));
	}

	PhysicsEngine::~PhysicsEngine()
	{
		delete dynamics_world;
		delete broadphase;
		delete collision_cfg;
	}

	auto PhysicsEngine::create_box(float const size[3], float const inertia[3], float mass, float const model_matrix[16])->Collidable*
	{
		auto collidable = new Collidable;

		collidable->shape = new btBoxShape(btVector3(size[0], size[1], size[2]));

		btTransform trans;
		trans.setIdentity();
		trans.setFromOpenGLMatrix(model_matrix);

		collidable->motion_state = new btDefaultMotionState(trans);

		collidable->body = new btRigidBody(btScalar(mass), collidable->motion_state, collidable->shape, btVector3(inertia[0], inertia[1], inertia[2]));

		dynamics_world->addRigidBody(collidable->body);

		return collidable;
	}

	auto PhysicsEngine::destroy_box(Collidable* collidable)->void
	{
		dynamics_world->removeRigidBody(collidable->body);

		delete collidable->body;
		delete collidable->shape;
		delete collidable->motion_state;

		delete collidable;
	}

	auto PhysicsEngine::get_model_matrix(Collidable* collidable, float* model_matrix)->void
	{
		btTransform trans;

		collidable->body->getMotionState()->getWorldTransform(trans);
		trans.getOpenGLMatrix(model_matrix);
	}

	auto PhysicsEngine::step(float dt, int max_steps)->void
	{
		dynamics_world->stepSimulation(dt, max_steps);
	}
}