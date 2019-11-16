#include "physics.h"

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <btBulletDynamicsCommon.h>

#include <iostream>

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

	auto PhysicsEngine::create_box(float mass, float const model_matrix[16])->Collidable*
	{
		auto collidable = new Collidable;

		collidable->shape = new btBoxShape(btVector3(1, 1, 1));

		glm::mat4 transformation = glm::make_mat4(model_matrix);
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transformation, scale, rotation, translation, skew, perspective);

		collidable->shape->setLocalScaling(btVector3(scale[0], scale[1], scale[2]));

		const auto new_transform = glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(glm::conjugate(rotation));

		btTransform trans;
		trans.setFromOpenGLMatrix(glm::value_ptr(new_transform));

		collidable->motion_state = new btDefaultMotionState(trans);

		btVector3 inertia(1, 1, 1);

		if (mass != 0)
		{
			collidable->shape->calculateLocalInertia(mass, inertia);
		}

		collidable->body = new btRigidBody(btScalar(mass), collidable->motion_state, collidable->shape, inertia);
		collidable->body->setFriction(0.65f);

		dynamics_world->addRigidBody(collidable->body);

		return collidable;
	}

	auto PhysicsEngine::create_sphere(float const size, float mass, float const model_matrix[16])->Collidable*
	{
		auto collidable = new Collidable;

		collidable->shape = new btSphereShape(size);

		btTransform trans;
		trans.setFromOpenGLMatrix(model_matrix);

		collidable->motion_state = new btDefaultMotionState(trans);

		btVector3 inertia(1, 1, 1);

		if (mass != 0)
		{
			collidable->shape->calculateLocalInertia(mass, inertia);
		}

		collidable->body = new btRigidBody(btScalar(mass), collidable->motion_state, collidable->shape, inertia);
		collidable->body->setFriction(0.65f);
		collidable->body->setRollingFriction(0.65f);

		dynamics_world->addRigidBody(collidable->body);

		return collidable;
	}

	auto PhysicsEngine::destroy_collidable(Collidable* collidable)->void
	{
		dynamics_world->removeRigidBody(collidable->body);

		delete collidable->body;
		delete collidable->shape;
		delete collidable->motion_state;

		delete collidable;
	}

	auto PhysicsEngine::get_model_matrix(Collidable* collidable, glm::mat4& model_matrix)->void
	{
		btTransform trans;
		collidable->motion_state->getWorldTransform(trans);

		const auto pos = trans.getOrigin();
		const auto rot = trans.getRotation();
		const auto scale = collidable->shape->getLocalScaling();

		model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(pos[0], pos[1], pos[2])) * glm::mat4_cast(glm::quat(rot[0], rot[1], rot[2], rot[3])) * glm::scale(glm::mat4(1.0), glm::vec3(scale[0] * 2, scale[1] * 2, scale[2] * 2));
	}

	auto PhysicsEngine::apply_force(Collidable* collidable, glm::vec3 force)->void
	{
		collidable->body->activate(true);
		collidable->body->applyCentralImpulse(btVector3(force[0], force[1], force[2]));
	}

	auto PhysicsEngine::raycast(glm::vec3 from, glm::vec3 to)->bool
	{
		const btVector3 src(from[0], from[1], from[2]);
		const btVector3 dst(to[0], to[1], to[2]);

		btCollisionWorld::ClosestRayResultCallback res(src, dst);

		dynamics_world->rayTest(src, dst, res);

		return res.hasHit();
	}

	auto PhysicsEngine::step(float dt, int max_steps)->void
	{
		dynamics_world->stepSimulation(dt, max_steps);
	}
}