#include "scene.h"
#include "gpu.h"
#include "physics.h"

#include <iostream>
#include <memory>

using namespace std;

/// TODO this shouldn't be void, make it return some information about WHERE in the Scene* this
/// model's GPU resources have been placed, so that future objects which use that model can simply
/// copy those regions from the Scene* and append them back into its self.
static void make_game_object(
	Scene* scene,
	gpu::ShaderPipeline* pipeline,
	gpu::RenderDevice* gpu,
	gengine::PhysicsEngine* physics_engine,
	gengine::TextureFactory* texture_factory,
	const glm::mat4& matrix,
	gengine::Collidable* rigidbody,
	gengine::SceneAsset model)
{

	std::vector<gpu::Descriptors*> descriptor_cache;
	std::vector<gpu::Geometry*> renderable_cache;

	/// Material --> Descriptors
	for (const auto& material : model.materials) {

		// TODO - move this inside assets.cpp
		gengine::ImageAsset texture_0{};
		if (material.textures.size() > 0) {
			texture_0 = material.textures[0];
		}
		else {
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
		if (rigidbody == nullptr) {
			// auto tr = glm::mat4{1.0f};
			auto tr = t;
			// tr = glm::rotate(tr, 3.14f, glm::vec3(0, 1, 0));
			scene->transforms.push_back(tr);
			scene->collidables.push_back(
				physics_engine->create_mesh(0.0f, model.geometries[geometry_idx], tr));
		}
		else {
			// scene->transforms.push_back(matrix);
			// scene->collidables.push_back(rigidbody);
		}
	}
}

SceneBuilder::SceneBuilder() {}

SceneBuilder::~SceneBuilder() {}

void SceneBuilder::add_game_object(
	const glm::mat4& matrix, TactileCapsule&& capsule, VisualModel&& model)
{
	game_objects.push_back(
		{.matrix = matrix,
		 .shape_type = TactileType::CAPSULE,
		 .shape_idx = capsule_shapes.size(),
		 .model_idx = models.size()});
	capsule_shapes.push_back(capsule);
	models.push_back(model);
}

void SceneBuilder::add_game_object(
	const glm::mat4& matrix, TactileSphere&& sphere, VisualModel&& model)
{
	game_objects.push_back(
		{.matrix = matrix,
		 .shape_type = TactileType::SPHERE,
		 .shape_idx = sphere_shapes.size(),
		 .model_idx = models.size()});
	sphere_shapes.push_back(sphere);
	models.push_back(model);
}

void SceneBuilder::add_game_object(const glm::mat4& matrix, VisualModel&& model)
{
	game_objects.push_back(
		{.matrix = matrix,
		 .shape_type = TactileType::MESH,
		 .shape_idx = models.size(),
		 .model_idx = models.size()});
	models.push_back(model);
}

unique_ptr<Scene> SceneBuilder::build(
	gpu::ShaderPipeline* pipeline,
	gpu::RenderDevice* gpu,
	gengine::PhysicsEngine* physics_engine,
	gengine::TextureFactory* texture_factory)
{
	/* Everything inside this function is horribly named. */
	auto scene = make_unique<Scene>();

	/// scene_path --> Engine's Scene IR
	unordered_map<string, gengine::SceneAsset> scene_cache;

	/// scene_path --> Engine's Renderable Scene IR
	unordered_map<string, Scene> render_scene_cache;

	// For each object we've queued,
	for (const GameObject& game_object : game_objects) {

		// Construct its rigid body using the physics engine
		gengine::Collidable* rigidbody = nullptr;
		switch (game_object.shape_type) {
		case TactileType::CAPSULE: {
			const auto details = capsule_shapes[game_object.shape_idx];
			rigidbody = physics_engine->create_capsule(details.mass, game_object.matrix);
			break;
		}
		case TactileType::SPHERE: {
			const auto details = sphere_shapes[game_object.shape_idx];
			rigidbody =
				physics_engine->create_sphere(details.radius, details.mass, game_object.matrix);
			break;
		}
		case TactileType::MESH: {
			rigidbody = nullptr;
			break;
		}
		default: {
			std::cout << "Error: unknown tactile type for scene object" << std::endl;
			assert(false);
		}
		}

		// If we haven't loaded this model from Assimp yet,
		if (!scene_cache.contains(models[game_object.model_idx].model_path)) {

			// Load this model
			const auto model = gengine::load_model(
				*texture_factory,
				models[game_object.model_idx].model_path,
				models[game_object.model_idx].flip_uvs,
				models[game_object.model_idx].flipWindingOrder);

			// Cache this model
			scene_cache[models[game_object.model_idx].model_path] = model;
		}

		// If we don't have a renderable scene for this model,
		if (!render_scene_cache.contains(models[game_object.model_idx].model_path)) {

			// Get model from cache
			const auto model = scene_cache[models[game_object.model_idx].model_path];

			// Create a blank Renderable Scene in the cache
			render_scene_cache[models[game_object.model_idx].model_path] = Scene{};

			// Instantiate the Renderable Scene by creating GPU resources
			make_game_object(
				&render_scene_cache[models[game_object.model_idx].model_path],
				pipeline,
				gpu,
				physics_engine,
				texture_factory,
				game_object.matrix,
				rigidbody,
				model);

			// Get this renderable scene from the cache
			const Scene& renderable_scene =
				render_scene_cache[models[game_object.model_idx].model_path];

			for (const auto& c : renderable_scene.collidables) {
				scene->res_collidables.push_back(c);
			}
			for (const auto& g : renderable_scene.render_components) {
				scene->res_render_components.push_back(g);
			}
			for (const auto& d : renderable_scene.descriptors) {
				scene->res_descriptors.push_back(d);
			}
		}

		// Get this renderable scene from the cache
		const Scene& renderable_scene =
			render_scene_cache[models[game_object.model_idx].model_path];

		// If rigidbody == nullptr, then we're generating collidables and transforms from the
		// imported scene.  So, let's grab them from there.
		if (rigidbody == nullptr) {
			for (const auto& c : renderable_scene.collidables) {
				scene->collidables.push_back(c);
			}
			for (const auto& t : renderable_scene.transforms) {
				scene->transforms.push_back(t);
			}
		}
		// Otherwise, we're providing our own collidable and transform from the game_object that the
		// API user described during the scene building phase.  Let's use that one instead.
		else {
			scene->transforms.push_back(game_object.matrix);
			scene->collidables.push_back(rigidbody);
		}

		// For render components and material descriptors, we can re-use whatever we created from
		// the function `make_game_object`.
		for (const auto& g : renderable_scene.render_components) {
			scene->render_components.push_back(g);
		}
		for (const auto& d : renderable_scene.descriptors) {
			scene->descriptors.push_back(d);
		}
	}

	return std::move(scene);
}