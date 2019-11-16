#include <GLFW/glfw3.h>
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "camera.hpp"
#include "physics.h"
#include "renderer/renderer.h"

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

/*
- NOTETAKING SECTION

builder.addPass([&](BuilderScope & scope)
{
	// construct the graph

	scope.read(some resource);
	scope.read(some other resource);
	scope.write(output resource);

	// execute the graph

	return [=](RenderGraphRegistry & registry, RenderCommandList & cmdlist)
	{
		cmdlist.draw(some stuff here);
	}
});

*/

namespace
{
	Camera camera(glm::vec3(0.0f, 5.0f, 90.0f));

	auto last_mouse_x = 0.0f;
	auto last_mouse_y = 0.0f;

	std::vector<glm::mat4> transforms;
	std::vector<gengine::Collidable*> collidables;

	auto mouse_callback(GLFWwindow* window, double pos_x, double pos_y) -> void
	{
		const auto offset_x = pos_x - last_mouse_x;
		const auto offset_y = last_mouse_y - pos_y;

		last_mouse_x = pos_x;
		last_mouse_y = pos_y;

		camera.ProcessMouseMovement(offset_x, offset_y);
	}

	auto load_file(std::string_view path)->std::string
	{
		std::ifstream stream(path.data(), std::ifstream::binary);
		
		std::stringstream buffer;
		
		buffer << stream.rdbuf();

		return buffer.str();
	}
}

auto main(int argc, char** argv) -> int
{
	/////
	// Physics
	/////

	gengine::PhysicsEngine physics_engine;

	{
		float const mass = 0.0f;

		glm::mat4 transform(1.0f);
		transform = glm::scale(transform, glm::vec3(100.0f, 1.0f, 100.0f));

		transforms.push_back(transform);
		collidables.push_back(physics_engine.create_box(mass, glm::value_ptr(transform)));
	}

	{
		float const mass = 2.0f;

		glm::mat4 transform(1.0f);
		transform = glm::translate(transform, glm::vec3(10.0f, 50.0f, 0.0f));
		transform = glm::scale(transform, glm::vec3(5.0f));

		transforms.push_back(transform);
		collidables.push_back(physics_engine.create_box(mass, glm::value_ptr(transform)));
	}

	{
		float const size = 1.78f;
		float const mass = 62.0f;

		glm::mat4 transform(1.0f);
		transform = glm::translate(transform, glm::vec3(20.0f, 30.0f, 20.0f));

		transforms.push_back(transform);
		collidables.push_back(physics_engine.create_sphere(size, mass, glm::value_ptr(transform)));
	}

	/////
	// Graphics
	/////

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	auto window = glfwCreateWindow(1280, 720, "Hello World", nullptr, nullptr);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);

	auto renderer = gengine::create_renderer(window);

	std::vector<float> vertices{};

	std::vector<unsigned int> indices{};

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile("mymodel.obj", aiProcess_Triangulate | aiProcess_GenNormals);

	//Check for errors
	if ((!scene) || (scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE) || (!scene->mRootNode))
	{
		std::cerr << "Error loading mymodel.obj: " << std::string(importer.GetErrorString()) << std::endl;
		//Return fail
		return -1;
	}

	//Iterate over the meshes
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		//Get the mesh
		aiMesh* mesh = scene->mMeshes[i];

		//Iterate over the vertices of the mesh
		for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
		{
			vertices.push_back(mesh->mVertices[j].x);
			vertices.push_back(mesh->mVertices[j].y);
			vertices.push_back(mesh->mVertices[j].z);

			vertices.push_back(mesh->mNormals[j].x);
			vertices.push_back(mesh->mNormals[j].y);
			vertices.push_back(mesh->mNormals[j].z);
		}

		//Iterate over the faces of the mesh
		for (unsigned int j = 0; j < mesh->mNumFaces; ++j)
		{
			//Get the face
			aiFace face = mesh->mFaces[j];
			//Add the indices of the face to the vector
			for (unsigned int k = 0; k < face.mNumIndices; ++k) { indices.push_back(face.mIndices[k]); }
		}
	}

	auto vertex_buffer = renderer->create_vertex_buffer(vertices, indices);

	auto vert = renderer->create_shader_module(load_file("../../data/cube.vert.spv"));
	auto frag = renderer->create_shader_module(load_file("../../data/cube.frag.spv"));

	auto pipeline = renderer->create_pipeline(vert, frag);

	renderer->destroy_shader_module(vert);
	renderer->destroy_shader_module(frag);

	while (!glfwWindowShouldClose(window))
	{
		physics_engine.step(1.0f / 1000.0f, 10);

		const auto view = camera.GetViewMatrix();

		for (auto i = 0; i < collidables.size(); ++i)
		{
			physics_engine.get_model_matrix(collidables[i], transforms[i]);
		}

		camera.Position = glm::vec3(transforms[2][3]);

		glfwPollEvents();

		const float sensitivity = 111.0f;

		if (glfwGetKey(window, GLFW_KEY_ESCAPE))
		{
			glfwSetWindowShouldClose(window, true);
		}

		if (glfwGetKey(window, GLFW_KEY_W))
		{
			physics_engine.apply_force(collidables[2], glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
		}
		else if (glfwGetKey(window, GLFW_KEY_S))
		{
			physics_engine.apply_force(collidables[2], glm::vec3(-camera.Front.x, 0.0f, -camera.Front.z));
		}
		if (glfwGetKey(window, GLFW_KEY_A))
		{
			physics_engine.apply_force(collidables[2], glm::vec3(-camera.Right.x, 0.0f, -camera.Right.z));
		}
		else if (glfwGetKey(window, GLFW_KEY_D))
		{
			physics_engine.apply_force(collidables[2], glm::vec3(camera.Right.x, 0.0f, camera.Right.z));
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE))
		{
			const auto on_ground = physics_engine.raycast(camera.Position, glm::vec3(camera.Position.x, camera.Position.y - 6, camera.Position.z));

			if (on_ground) physics_engine.apply_force(collidables[2], glm::vec3(0.0f, 5.6f, 0.0f));
		}

		if (glfwGetKey(window, GLFW_KEY_LEFT))
		{
			camera.ProcessMouseMovement(-0.25f, 0.0f);
		}
		else if (glfwGetKey(window, GLFW_KEY_RIGHT))
		{
			camera.ProcessMouseMovement(0.25f, 0.0f);
		}
		if (glfwGetKey(window, GLFW_KEY_UP))
		{
			camera.ProcessMouseMovement(0.0f, 0.25f);
		}
		else if (glfwGetKey(window, GLFW_KEY_DOWN))
		{
			camera.ProcessMouseMovement(0.0f, -0.25f);
		}

		auto cmdlist = renderer->alloc_cmdlist();
		
		cmdlist->start_recording();

		cmdlist->start_frame(pipeline);

		cmdlist->bind_pipeline(pipeline);

		cmdlist->bind_vertex_buffer(vertex_buffer);

		for (const auto& transform : transforms)
		{
			cmdlist->push_constants(pipeline, transform, glm::value_ptr(view));

			cmdlist->draw(36, 1);
		}

		cmdlist->end_frame();

		cmdlist->stop_recording();

		renderer->execute_cmdlist(cmdlist);

		renderer->free_cmdlist(cmdlist);

		glfwSwapBuffers(window);
	}

	for (const auto& collidable : collidables)
	{
		physics_engine.destroy_collidable(collidable);
	}

	renderer->destroy_pipeline(pipeline);

	renderer->destroy_vertex_buffer(vertex_buffer);

	gengine::destroy_renderer(renderer);

	glfwDestroyWindow(window);

	glfwTerminate();
}