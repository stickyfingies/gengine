#pragma once

#include "gpu.h"
#include "physics.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <unordered_set>

struct ResourceContainer {

	/**
	 * A set which takes ownership of resources whose destruction order does not matter.
	 * @tparam Resource some resource.
	 */
	template <class Resource> using ResourceSet = std::unordered_set<Resource>;

	ResourceSet<gengine::Collidable*> rigidbodies;
	ResourceSet<gpu::Descriptors*> gpu_descriptors;
	ResourceSet<gpu::Geometry*> gpu_geometries;
	ResourceSet<gpu::Image*> gpu_images;
};

/**
 * @brief A container for everything we need to simulate and render a game scene.
 */
struct Scene {
	// The Scene implements an index-based entity system.
	// Each entity in the scene has one slot in each of the below vectors.
	// The resources in the vectors "belong" to the res_* equivalents below.
	std::vector<glm::mat4> transforms{};
	std::vector<gengine::Collidable*> collidables{};
	std::vector<gpu::Geometry*> render_components{};
	std::vector<gpu::Descriptors*> descriptors{};
};

/// Building block used for creating Capsule shapes.
struct TactileCapsule {
	float mass;
};

/// Building block used for creating Sphere shapes.
struct TactileSphere {
	float mass;
	float radius;
};

/// Building block used for creating game objects from 3D model files.
struct VisualModel {
	std::string path;
};

/// Settings we apply while processing a 3D model file
struct VisualModelSettings {
	bool flip_uvs;
	bool flip_triangle_winding;
	bool make_rigidbody;
};

/**
 * @brief \c SceneBuilder is an interface for describing and creating a \c Scene.
 */
class SceneBuilder {
public:
	SceneBuilder();
	~SceneBuilder();

	SceneBuilder(const SceneBuilder&) = delete;
	SceneBuilder& operator=(const SceneBuilder&) = delete;

	/// Adds a tangible capsule to the scene, with some visual model applied
	void add_game_object(const glm::mat4& matrix, TactileCapsule&&, VisualModel&&);

	/// Adds a tangible sphere to the scene, with some visual model applied
	void add_game_object(const glm::mat4& matrix, TactileSphere&&, VisualModel&&);

	/// Adds a tangible model to the scene
	void add_game_object(const glm::mat4& matrix, VisualModel&&);

	void apply_model_settings(const std::string& model_path, VisualModelSettings&&);

	/**
	 * @brief Actualizes the Scene we've been building so far.
	 *
	 * @param pipeline raster pipeline to use for building materials
	 * @param gpu device to use for creating GPU resources
	 * @param physics_engine used for creating collidable shapes
	 * @param texture_factory used for loading textures etc
	 * @return A fully built Scene object
	 */
	std::unique_ptr<Scene> build(
		ResourceContainer& resources,
		gpu::ShaderPipelineHandle pipeline,
		gpu::RenderDevice* gpu,
		gengine::PhysicsEngine* physics_engine,
		gengine::TextureFactory* texture_factory);

private:
	/// Different types of collision shapes
	enum class TactileType { CAPSULE, SPHERE, MESH };

	/// Describes a game object that we'll build eventually
	struct GameObject {
		glm::mat4 matrix;
		TactileType shape_type;
		size_t shape_idx;
		size_t model_idx;
	};

	/// Growable list of game objects, to be built later
	std::vector<GameObject> game_objects;

	/// Growable list of shape descriptions, to be built later
	std::vector<TactileCapsule> capsule_shapes;

	/// Growable list of shape descriptions, to be built later
	std::vector<TactileSphere> sphere_shapes;

	/// Growable list of model descriptions, to be built later
	std::vector<VisualModel> models;

	/// Hashmap of model_path --> model_settings
	std::unordered_map<std::string, VisualModelSettings> model_settings_storage;
};
