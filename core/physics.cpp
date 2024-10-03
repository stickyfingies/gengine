#include "physics.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <bullet/btBulletDynamicsCommon.h>

#include <iostream>

namespace gengine {
struct Collidable {
	std::unique_ptr<btTriangleMesh> mesh;
	std::unique_ptr<btMotionState> motion_state;
	std::unique_ptr<btCollisionShape> shape;
	std::unique_ptr<btRigidBody> body;
	glm::vec3 scale;
};

PhysicsEngine::PhysicsEngine()
{
	std::cout << "[info]\t Intitializing physics engine" << std::endl;

	collision_cfg = std::make_unique<btDefaultCollisionConfiguration>();

	broadphase = std::make_unique<btDbvtBroadphase>();

	dynamics_world = std::make_unique<btDiscreteDynamicsWorld>(
		new btCollisionDispatcher(collision_cfg.get()),
		broadphase.get(),
		new btSequentialImpulseConstraintSolver(),
		collision_cfg.get());

	dynamics_world->setGravity(btVector3(0, -9.8, 0));
}

PhysicsEngine::~PhysicsEngine() {}

auto PhysicsEngine::create_box(float mass, const glm::mat4& model_matrix) -> Collidable*
{
	auto collidable = new Collidable{};

	auto scale = glm::vec3{};
	auto rotation = glm::quat{};
	auto translation = glm::vec3{};
	auto skew = glm::vec3{};
	auto perspective = glm::vec4{};
	glm::decompose(model_matrix, scale, rotation, translation, skew, perspective);

	collidable->scale = scale;
	collidable->shape = std::make_unique<btBoxShape>(btVector3(scale.x, scale.y, scale.z));

	const auto new_transform =
		glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(glm::conjugate(rotation));

	auto trans = btTransform{};
	trans.setFromOpenGLMatrix(glm::value_ptr(new_transform));

	collidable->motion_state = std::make_unique<btDefaultMotionState>(trans);

	auto inertia = btVector3(1, 1, 1);

	if (mass != 0) {
		collidable->shape->calculateLocalInertia(mass, inertia);
	}

	collidable->body = std::make_unique<btRigidBody>(
		btScalar(mass), collidable->motion_state.get(), collidable->shape.get(), inertia);
	collidable->body->setFriction(0.4);
	collidable->body->setRollingFriction(0.3);
	collidable->body->setSpinningFriction(0.3);

	dynamics_world->addRigidBody(collidable->body.get());

	return collidable;
}

auto PhysicsEngine::create_sphere(float const size, float mass, const glm::mat4& model_matrix)
	-> Collidable*
{
	auto collidable = new Collidable{};

	auto scale = glm::vec3{};
	auto rotation = glm::quat{};
	auto translation = glm::vec3{};
	auto skew = glm::vec3{};
	auto perspective = glm::vec4{};
	glm::decompose(model_matrix, scale, rotation, translation, skew, perspective);

	collidable->scale = scale;
	collidable->shape = std::make_unique<btSphereShape>((scale.x + scale.y + scale.z) / 3.0f);

	auto trans = btTransform{};
	trans.setFromOpenGLMatrix(glm::value_ptr(model_matrix));

	collidable->motion_state = std::make_unique<btDefaultMotionState>(trans);

	auto inertia = btVector3(1, 1, 1);

	if (mass != 0) {
		collidable->shape->calculateLocalInertia(mass, inertia);
	}

	collidable->body = std::make_unique<btRigidBody>(
		btScalar(mass), collidable->motion_state.get(), collidable->shape.get(), inertia);
	collidable->body->setFriction(0.3);
	collidable->body->setRollingFriction(0.3);
	collidable->body->setSpinningFriction(0.3);

	dynamics_world->addRigidBody(collidable->body.get());

	return collidable;
}

auto PhysicsEngine::create_capsule(float mass, const glm::mat4& model_matrix) -> Collidable*
{
	auto collidable = new Collidable{};

	auto scale = glm::vec3{};
	auto rotation = glm::quat{};
	auto translation = glm::vec3{};
	auto skew = glm::vec3{};
	auto perspective = glm::vec4{};
	glm::decompose(model_matrix, scale, rotation, translation, skew, perspective);

	collidable->scale = scale;
	collidable->shape = std::make_unique<btCapsuleShape>(4.0, 1.7);

	auto trans = btTransform{};
	trans.setFromOpenGLMatrix(glm::value_ptr(model_matrix));

	collidable->motion_state = std::make_unique<btDefaultMotionState>(trans);

	auto inertia = btVector3(1, 1, 1);

	if (mass != 0) {
		collidable->shape->calculateLocalInertia(mass, inertia);
	}

	collidable->body = std::make_unique<btRigidBody>(
		btScalar(mass), collidable->motion_state.get(), collidable->shape.get(), inertia);
	collidable->body->setFriction(0.3);
	collidable->body->setAngularFactor(0.0);

	dynamics_world->addRigidBody(collidable->body.get());

	return collidable;
}

auto PhysicsEngine::create_mesh(
	float mass, const GeometryAsset& geometry, const glm::mat4& model_matrix) -> Collidable*
{
	auto collidable = new Collidable{};

	auto scale = glm::vec3{};
	auto rotation = glm::quat{};
	auto translation = glm::vec3{};
	auto skew = glm::vec3{};
	auto perspective = glm::vec4{};
	glm::decompose(model_matrix, scale, rotation, translation, skew, perspective);

	// Convert our optimized (vertex, index) buffer into non-optimized (vertex) buffer

	const auto& vertices = geometry.vertices;
	const auto& indices = geometry.indices;

	auto nonIndexedVertices = std::vector<float>();
	for (int i = 0; i < indices.size(); i++) {
		const auto idx = indices[i];
		nonIndexedVertices.push_back(-vertices[idx * 3 + 0]);
		nonIndexedVertices.push_back(vertices[idx * 3 + 1]);
		nonIndexedVertices.push_back(vertices[idx * 3 + 2]);
	}

	std::cout << "[info]\t Collidable (" << vertices.size() << " vertices, "
			  << nonIndexedVertices.size() << " non-indexed)" << std::endl;

	// Populate a triangle mesh using our non-optimized vertex buffer

	collidable->mesh = std::make_unique<btTriangleMesh>();
	for (int i = 0; i < nonIndexedVertices.size(); i += 9) {
		const auto v0 = btVector3(
			nonIndexedVertices[i + 0], nonIndexedVertices[i + 1], nonIndexedVertices[i + 2]);
		const auto v1 = btVector3(
			nonIndexedVertices[i + 3], nonIndexedVertices[i + 4], nonIndexedVertices[i + 5]);
		const auto v2 = btVector3(
			nonIndexedVertices[i + 6], nonIndexedVertices[i + 7], nonIndexedVertices[i + 8]);
		collidable->mesh->addTriangle(v0, v1, v2, true);
	}

	// // auto indexed_mesh = btIndexedMesh{};
	// // indexed_mesh.m_numTriangles = indices.size() / 3;
	// // indexed_mesh.m_numVertices = vertices.size() / 3;
	// // indexed_mesh.m_triangleIndexBase = reinterpret_cast<const unsigned char*>(indices.data());
	// // indexed_mesh.m_vertexBase = reinterpret_cast<const unsigned char*>(vertices.data());
	// // indexed_mesh.m_triangleIndexStride = 3 * sizeof(unsigned int);
	// // indexed_mesh.m_vertexStride = 3 * sizeof(float);
	// // indexed_mesh.m_vertexType = PHY_FLOAT;

	// collidable->mesh = new btTriangleIndexVertexArray();
	// collidable->mesh->addIndexedMesh(indexed_mesh, PHY_INTEGER);

	collidable->scale = scale;
	collidable->shape = std::make_unique<btBvhTriangleMeshShape>(collidable->mesh.get(), true);
	collidable->shape->setLocalScaling(btVector3(scale.x, scale.y, scale.z));

	auto trans = btTransform{};
	trans.setIdentity();
	trans.setOrigin(btVector3{translation.x, translation.y, translation.z});
	trans.setRotation(btQuaternion{rotation.x, rotation.y, rotation.z, rotation.w});
	trans.setFromOpenGLMatrix(glm::value_ptr(model_matrix));

	collidable->motion_state = std::make_unique<btDefaultMotionState>(trans);

	auto inertia = btVector3(1, 1, 1);
	if (mass != 0) {
		collidable->shape->calculateLocalInertia(mass, inertia);
	}

	collidable->body = std::make_unique<btRigidBody>(
		mass, collidable->motion_state.get(), collidable->shape.get(), inertia);
	collidable->body->setFriction(0.3);
	collidable->body->setAngularFactor(0.0);

	dynamics_world->addRigidBody(collidable->body.get());

	return collidable;
}

auto PhysicsEngine::destroy_collidable(Collidable* collidable) -> void
{
	dynamics_world->removeRigidBody(collidable->body.get());

	delete collidable;
}

auto PhysicsEngine::get_model_matrix(Collidable* collidable, glm::mat4& model_matrix) -> void
{
	auto trans = btTransform{};
	collidable->motion_state->getWorldTransform(trans);

	const auto pos = trans.getOrigin();
	const auto rot = trans.getRotation();
	const auto scale = collidable->scale;

	model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(pos[0], pos[1], pos[2])) *
				   glm::mat4_cast(glm::quat(rot[0], rot[1], rot[2], rot[3])) *
				   glm::scale(glm::mat4(1.0), scale);
}

auto PhysicsEngine::apply_force(Collidable* collidable, glm::vec3 force) -> void
{
	collidable->body->activate(true);
	collidable->body->applyCentralImpulse(btVector3(force[0], force[1], force[2]));
}

auto PhysicsEngine::raycast(glm::vec3 from, glm::vec3 to) -> bool
{
	const auto src = btVector3(from[0], from[1], from[2]);
	const auto dst = btVector3(to[0], to[1], to[2]);

	auto res = btCollisionWorld::ClosestRayResultCallback(src, dst);

	dynamics_world->rayTest(src, dst, res);

	return res.hasHit();
}

auto PhysicsEngine::step(float dt, int max_steps) -> void
{
	dynamics_world->stepSimulation(dt, max_steps);
}
} // namespace gengine
