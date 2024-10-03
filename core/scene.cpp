#include "scene.h"
#include "gpu.h"
#include "physics.h"

#include <iostream>
#include <memory>
using namespace std;

SceneBuilder::SceneBuilder(
	gpu::RenderDevice* gpu,
	gengine::PhysicsEngine* physics_engine,
	gengine::TextureFactory* texture_factory)
	: texture_factory{texture_factory}, gpu{gpu}, physics_engine{physics_engine}
{
	scene = make_unique<Scene>();
}

SceneBuilder::~SceneBuilder()
{
	//
}

void SceneBuilder::add_game_object(
	gpu::ShaderPipeline* pipeline,
	const glm::mat4& matrix,
	gengine::Collidable* rigidbody,
	std::string_view model_path,
	bool makeMesh,
	bool flipUVs,
	bool flipWindingOrder)
{

	if (rigidbody == nullptr && makeMesh == false) {
		cout << "Error: Scene builder must have a rigidbody to make a game object." << endl;
		return;
	}

	std::vector<gpu::Descriptors*> descriptor_cache;
	std::vector<gpu::Geometry*> renderable_cache;

	const auto model = gengine::load_model(*texture_factory, model_path, flipUVs, flipWindingOrder);

	/// Material --> Descriptors
	for (const auto& material : model.materials) {

		// TODO - move this inside assets.cpp
		gengine::ImageAsset texture_0{};
		if (material.textures.size() > 0) {
			texture_0 = material.textures[0];
		}
		if (texture_0.width == 0) {
			texture_0 = *texture_factory->load_image_from_file("./data/Albedo.png");
		}

		auto albedo = gpu->create_image(
			texture_0.name,
			texture_0.width,
			texture_0.height,
			texture_0.channel_count,
			texture_0.data);

		const auto descriptor_0 = gpu->create_descriptors(pipeline, albedo, material.color);

		descriptor_cache.push_back(descriptor_0);
	}

	/// Geometry --> Renderable
	for (const auto& geometry : model.geometries) {
		const auto renderable =
			gpu->create_geometry(geometry.vertices, geometry.vertices_aux, geometry.indices);
		renderable_cache.push_back(renderable);
	}

	// Game objects
	for (const auto& object : model.objects) {
		const auto& [t, geometry_idx, material_idx] = object;

		std::cout << "Creating object: " << geometry_idx << ", " << material_idx << " | "
				  << renderable_cache.size() << ", " << descriptor_cache.size() << std::endl;

		scene->render_components.push_back(renderable_cache[geometry_idx]);

		// Materials

		scene->descriptors.push_back(descriptor_cache[material_idx]);

		// gengine::unload_image(texture_0);

		// Generate a physics model when makeMesh == true
		if (makeMesh) {
			// auto tr = glm::mat4{1.0f};
			auto tr = t;
			// tr = glm::rotate(tr, 3.14f, glm::vec3(0, 1, 0));
			scene->transforms.push_back(tr);
			scene->collidables.push_back(
				physics_engine->create_mesh(0.0f, model.geometries[geometry_idx], tr));
			cout << "MADE STATIC BODY" << endl;
		}
		else {
			cout << "MADE BODY" << endl;
			scene->transforms.push_back(matrix);
			scene->collidables.push_back(rigidbody);
		}
	}
}

unique_ptr<Scene> SceneBuilder::get_scene() { return std::move(scene); }