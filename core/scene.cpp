#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "scene.h"
#include "gpu.h"
#include "physics.h"
#include "tiny_gltf.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <unordered_set>

using namespace std;

// A dictionary to cache GPU images that we've seen before
using GpuImageIndex = std::unordered_map<std::string, gpu::Image*>;

/// TODO this shouldn't be void, make it return some information about WHERE in the Scene* this
/// model's GPU resources have been placed, so that future objects which use that model can simply
/// copy those regions from the Scene* and append them back into its self.
static ResourceContainer make_game_object(
	ResourceContainer& global_resources,
	gpu::ShaderPipeline* pipeline,
	gpu::RenderDevice* gpu,
	gengine::TextureFactory* texture_factory,
	GpuImageIndex& gpu_image_index,
	const gengine::SceneAsset& model)
{
	cout << "Creating ResourceContainer for " << model.path << endl;
	ResourceContainer local_resources;

	/// For each material in this model...
	for (const auto& material : model.materials) {

		// Load its texture, using a default (if needed)
		gengine::ImageAsset texture_0{};
		if (material.textures.size() > 0) {
			texture_0 = material.textures[0];
		}
		else {
			texture_0 = *texture_factory->load_image_from_file("./data/Albedo.png");
		}

		// Create GPU image (if we haven't already)
		if (!gpu_image_index.contains(texture_0.name)) {
			const auto albedo = gpu->create_image(
				texture_0.name,
				texture_0.width,
				texture_0.height,
				texture_0.channel_count,
				texture_0.data);
			global_resources.gpu_images.insert(albedo);
			gpu_image_index[texture_0.name] = albedo;
		}
		const auto albedo = gpu_image_index[texture_0.name];

		// Create descriptor
		const auto descriptor_0 = gpu->create_descriptors(pipeline, albedo, material.color);

		global_resources.gpu_descriptors.insert(descriptor_0);
		local_resources.gpu_descriptors.insert(descriptor_0);
	}

	/// Geometry --> Renderable
	for (const auto& geometry : model.geometries) {
		const auto gpu_geometry =
			gpu->create_geometry(geometry.vertices, geometry.vertices_aux, geometry.indices);
		global_resources.gpu_geometries.insert(gpu_geometry);
		local_resources.gpu_geometries.insert(gpu_geometry);
	}

	// Game objects

	cout << "ResourceContainer completed" << endl;

	return local_resources;
}

struct RigidBodySet {
	std::vector<glm::mat4> transforms;
	std::vector<gengine::Collidable*> rigidbodies;
};

static RigidBodySet
make_rigidbody_from_model(gengine::PhysicsEngine* physics_engine, const gengine::SceneAsset& model)
{
	RigidBodySet rbs;
	for (const auto& object : model.objects) {
		const auto& [t, geometry_idx, material_idx] = object;
		rbs.transforms.push_back(t);
		rbs.rigidbodies.push_back(
			physics_engine->create_mesh(0.0f, model.geometries[geometry_idx], t));
	}
	return rbs;
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
	model_settings_storage[model.path].make_rigidbody = true;
}

void SceneBuilder::apply_model_settings(
	const std::string& model_path, VisualModelSettings&& settings)
{
	model_settings_storage[model_path] = settings;
}

/**
 * Docs with example:
 * https://github.com/syoyo/tinygltf/blob/release/examples/basic/main.cpp#L162
 */
void load_model_gltf(std::string_view model_path_raw, bool flip_uvs, bool flip_triangle_winding)
{
	const string model_path = filesystem::current_path() / model_path_raw;

	tinygltf::TinyGLTF loader;
	tinygltf::Model gltf_model;
	string err, warn;

	bool res = loader.LoadBinaryFromFile(&gltf_model, &err, &warn, model_path);
	if (!warn.empty()) {
		cout << "Warning while parsing glTF model (" << model_path << "): " << warn << endl;
	}
	if (!err.empty()) {
		cout << "Error while parsing glTF model (" << model_path << "): " << err << endl;
	}
	if (!res) {
		cout << "Failed to load glTF: " << model_path << endl;
		return;
	}
	cout << "glTF loaded from file: " << model_path << endl;
	const tinygltf::Scene& gltf_scene = gltf_model.scenes[gltf_model.defaultScene];

	// Process model buffers
	const int TINYGLTF_UNKNOWN_TARGET = 0;
	const int TINYGLTF_VERTEX_BUFFER_TARGET = 34962;
	const int TINYGLTF_INDEX_BUFFER_TARGET = 34963;
	for (tinygltf::BufferView& gltf_buffer_view : gltf_model.bufferViews) {
		const tinygltf::Buffer& gltf_buffer = gltf_model.buffers[gltf_buffer_view.buffer];

		switch (gltf_buffer_view.target) {
		case TINYGLTF_UNKNOWN_TARGET:
			break;
		case TINYGLTF_VERTEX_BUFFER_TARGET: {
			cout << "glTF decoded a vertex buffer." << endl;
			
			break;
		}
		case TINYGLTF_INDEX_BUFFER_TARGET: {
			cout << "glTF decoded an index buffer." << endl;
			break;
		}
		default:
			cerr << "Error in glTF decoder: unknown value for tinygltf::BufferView::target "
				 << "\"" << gltf_buffer_view.target << "\"" << endl;
			return;
		}
	}

	// Process scene nodes
	for (size_t gltf_node_idx : gltf_scene.nodes) {
		assert((gltf_node_idx >= 0) && (gltf_node_idx < gltf_model.nodes.size()));
		const tinygltf::Node& gltf_node = gltf_model.nodes[gltf_node_idx];

		// If: this node has a mesh.
		if (gltf_node.mesh >= 0 && gltf_node.mesh < gltf_model.meshes.size()) {
			const tinygltf::Mesh& gltf_mesh = gltf_model.meshes[gltf_node.mesh];
			// TODO(Seth) - process each mesh in the node
		}
		// Else if: this node has no mesh.
		else {
			// Tell me.  I'm curious if this ever happens.
			// TODO(Seth) - delete after investigating
			cout << "glTF decoder found a node without a mesh." << endl;
		}
	}
}

unique_ptr<Scene> SceneBuilder::build(
	ResourceContainer& resources,
	gpu::ShaderPipeline* pipeline,
	gpu::RenderDevice* gpu,
	gengine::PhysicsEngine* physics_engine,
	gengine::TextureFactory* texture_factory)
{
	/* Everything inside this function is horribly named. */
	auto scene = make_unique<Scene>();

	/// scene path --> Scene object
	/// this bridges phase 1 and 2
	unordered_map<string, ResourceContainer> asset_resource_lookup;

	/// scene path --> (tranform[], rigidbody[])
	/// this bridges phase 1 and 2
	unordered_map<string, RigidBodySet> rigid_body_storage;

	////
	// Phase 1: process 3D assets
	////

	GpuImageIndex gpu_image_index;

	// For each 3D model used in this scene...
	for (const auto& [model_path, model_settings] : model_settings_storage) {

		// TODO(Seth) - temporary code to try loading .glTF model
		load_model_gltf(model_path, model_settings.flip_uvs, model_settings.flip_triangle_winding);

		// Load this model
		const auto model = gengine::load_model(
			*texture_factory,
			model_path,
			model_settings.flip_uvs,
			model_settings.flip_triangle_winding);

		// Generate rigid bodies (if necessary)
		bool make_rigidbody = model_settings_storage.at(model_path).make_rigidbody;
		if (make_rigidbody) {
			rigid_body_storage[model_path] = make_rigidbody_from_model(physics_engine, model);
		}

		// Instantiate the Renderable Scene by creating GPU resources
		asset_resource_lookup[model_path] =
			make_game_object(resources, pipeline, gpu, texture_factory, gpu_image_index, model);
		const ResourceContainer& asset_resources = asset_resource_lookup[model_path];
	}

	////
	// Phase 2: use processed 3D assets to create game objects
	////

	// For each object we've queued,
	for (const GameObject& game_object : game_objects) {

		const auto& model_path = models[game_object.model_idx].path;

		// If we haven't loaded this model from Assimp yet,
		if (!asset_resource_lookup.contains(model_path)) {
			cout << "Error: unrecognized scene path " << model_path << endl;
			continue;
		}

		// Get this renderable scene from the cache
		const ResourceContainer& asset_resources =
			asset_resource_lookup[models[game_object.model_idx].path];

		///
		// Inserting rigidbodies into the scene has two modes of action.
		// (1) Generate them from a 3D model, which may produce multiple rigidbodies.
		// (2) Create them from a shape primitive, which produces one rigidbody.
		//
		// Generate a rigidbody...
		if (game_object.shape_type == TactileType::MESH) {

			// Ensure this mesh has been previously processed into a rigidbody
			if (!rigid_body_storage.contains(model_path)) {
				cout << "Error: trying to use generated rigidbody from a model " << model_path
					 << " which was not configured for rigidbody generation." << endl;
				continue;
			}
			// Grab the cached rigidbodies and append them hoes
			// TODO: we should be making rigidbodies once-per- game entity (i.e. on site).
			// TODO: rigid_body_storage should store shape info.
			const auto& rigidbodies = rigid_body_storage.at(model_path);
			for (const auto& transform : rigidbodies.transforms) {
				scene->transforms.push_back(transform);
			}
			for (const auto& rigidbody : rigidbodies.rigidbodies) {
				scene->collidables.push_back(rigidbody);
			}
		}
		// Else, create shape primitive...
		else {
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
			default: {
				std::cout << "Error: unknown tactile type for scene object" << std::endl;
				assert(false);
			}
			}
			scene->collidables.push_back(rigidbody);
			scene->transforms.push_back(game_object.matrix);
		}

		// For render components and material descriptors, we can re-use whatever we created from
		// the function `make_game_object`.
		for (const auto& g : asset_resources.gpu_geometries) {
			scene->render_components.push_back(g);
		}
		for (const auto& d : asset_resources.gpu_descriptors) {
			scene->descriptors.push_back(d);
		}
	}

	return std::move(scene);
}