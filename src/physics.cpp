#include "physics.h"

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <bullet/btBulletDynamicsCommon.h>

#include <iostream>

namespace gengine
{
	struct Collidable
	{
		btTriangleIndexVertexArray* mesh;
		btMotionState* motion_state;
		btCollisionShape* shape;
		btRigidBody* body;
		glm::vec3 scale;
	};

	PhysicsEngine::PhysicsEngine()
	{
		collision_cfg = new btDefaultCollisionConfiguration();

		broadphase = new btDbvtBroadphase();

		dynamics_world = new btDiscreteDynamicsWorld(new btCollisionDispatcher(collision_cfg), broadphase, new btSequentialImpulseConstraintSolver(), collision_cfg);

		dynamics_world->setGravity(btVector3(0, -9.8, 0));
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

		glm::mat4 transformation = glm::make_mat4(model_matrix);
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transformation, scale, rotation, translation, skew, perspective);

		collidable->scale = scale;
		collidable->shape = new btBoxShape(btVector3(scale.x, scale.y, scale.z));

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
		collidable->body->setFriction(0.4);
		collidable->body->setRollingFriction(0.3);
		collidable->body->setSpinningFriction(0.3);

		dynamics_world->addRigidBody(collidable->body);

		return collidable;
	}

	auto PhysicsEngine::create_sphere(float const size, float mass, float const model_matrix[16])->Collidable*
	{
		auto collidable = new Collidable;

		glm::mat4 transformation = glm::make_mat4(model_matrix);
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transformation, scale, rotation, translation, skew, perspective);

		collidable->scale = scale;
		collidable->shape = new btSphereShape((scale.x + scale.y + scale.z) / 3.0f);

		btTransform trans;
		trans.setFromOpenGLMatrix(model_matrix);

		collidable->motion_state = new btDefaultMotionState(trans);

		btVector3 inertia(1, 1, 1);

		if (mass != 0)
		{
			collidable->shape->calculateLocalInertia(mass, inertia);
		}

		collidable->body = new btRigidBody(btScalar(mass), collidable->motion_state, collidable->shape, inertia);
		collidable->body->setFriction(0.3);
		collidable->body->setRollingFriction(0.3);
		collidable->body->setSpinningFriction(0.3);

		dynamics_world->addRigidBody(collidable->body);

		return collidable;
	}

	auto PhysicsEngine::create_capsule(float mass, float const model_matrix[16])->Collidable*
	{
		auto collidable = new Collidable;

		glm::mat4 transformation = glm::make_mat4(model_matrix);
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transformation, scale, rotation, translation, skew, perspective);

		collidable->scale = scale;
		collidable->shape = new btCapsuleShape(4.0, 1.7);

		btTransform trans;
		trans.setFromOpenGLMatrix(model_matrix);

		collidable->motion_state = new btDefaultMotionState(trans);

		btVector3 inertia(1, 1, 1);

		if (mass != 0)
		{
			collidable->shape->calculateLocalInertia(mass, inertia);
		}

		collidable->body = new btRigidBody(btScalar(mass), collidable->motion_state, collidable->shape, inertia);
		collidable->body->setFriction(0.3);
		collidable->body->setAngularFactor(0.0);

		dynamics_world->addRigidBody(collidable->body);

		return collidable;
	}

	auto PhysicsEngine::create_mesh(float mass, const std::vector<float>& vertices, const std::vector<unsigned int>& indices, const float model_matrix[16]) -> Collidable*
	{
		auto collidable = new Collidable;

		glm::mat4 transformation = glm::make_mat4(model_matrix);
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transformation, scale, rotation, translation, skew, perspective);

		collidable->mesh = new btTriangleIndexVertexArray;

		btIndexedMesh indexed_mesh;
		indexed_mesh.m_indexType = PHY_INTEGER;
		indexed_mesh.m_numTriangles = indices.size() / 3;
		indexed_mesh.m_numVertices = vertices.size();
		indexed_mesh.m_triangleIndexBase = reinterpret_cast<const unsigned char*>(indices.data());
		indexed_mesh.m_triangleIndexStride = 3 * sizeof(unsigned int);
		indexed_mesh.m_vertexBase = reinterpret_cast<const unsigned char*>(vertices.data());
		indexed_mesh.m_vertexStride = 32;
		indexed_mesh.m_vertexType = PHY_FLOAT;

		collidable->mesh->addIndexedMesh(indexed_mesh);

		collidable->scale = scale;
		collidable->shape = new btBvhTriangleMeshShape(collidable->mesh, false);
		collidable->shape->setLocalScaling(btVector3(1, -1, 1));

		btTransform trans;
		trans.setFromOpenGLMatrix(model_matrix);

		collidable->motion_state = new btDefaultMotionState(trans);

		btVector3 inertia(1, 1, 1);

		if (mass != 0)
		{
			collidable->shape->calculateLocalInertia(mass, inertia);
		}

		collidable->body = new btRigidBody(mass, collidable->motion_state, collidable->shape, inertia);
		collidable->body->setFriction(0.3);
		collidable->body->setAngularFactor(0.0);

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
		const auto scale = collidable->scale;

		model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(pos[0], pos[1], pos[2])) * glm::mat4_cast(glm::quat(rot[0], rot[1], rot[2], rot[3])) * glm::scale(glm::mat4(1.0), scale);
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
