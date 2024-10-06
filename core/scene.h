#pragma once

#include "gpu.h"
#include "physics.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

/**
 * @brief Use \c SceneBuilder (see below) to create a Scene.
 */
struct Scene {
	std::vector<glm::mat4> transforms{};
	std::vector<gengine::Collidable*> collidables{};
	std::vector<gpu::Geometry*> render_components{};
	std::vector<gpu::Descriptors*> descriptors{};
};

/// Building block used for creating Capusle shapes.
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
	std::string_view model_path;
	bool flip_uvs;
	bool flipWindingOrder;
};

/**
 * @brief \c SceneBuilder is an interface for describing and creating a \c Scene.
 */
class SceneBuilder {

	/// The scene to build
	std::unique_ptr<Scene> scene;

	/// Different types of collision shapes
	enum class TactileType { CAPSULE, SPHERE, MESH };

	/// Describes a game object to be created
	struct GameObject {
		glm::mat4 matrix;
		TactileType shape_type;
		size_t shape_idx;
		size_t model_idx;
	};

	/// Growable list of game objects, to be actualized later
	std::vector<GameObject> game_objects;

	/// Growable list of shape descriptions, to be actualized later
	std::vector<TactileCapsule> capsule_shapes;

	/// Growable list of shape descriptions, to be actualized later
	std::vector<TactileSphere> sphere_shapes;

	/// Growable list of model descriptions, to be actualized later
	std::vector<VisualModel> models;

public:
	SceneBuilder();
	~SceneBuilder();

	/**
	 * @brief Introduce a new game object into a \c Scene.
	 *
	 * @param pipeline raster pipeline
	 * @param matrix world-space transform
	 * @param rigidbody collidable model, use 'nullptr' to generate from 3D model asset
	 * @param model_path path to the 3D model asset
	 * @param flipUVs when 'true' flip textures along the Y-axis
	 * @param flipWindingOrder fiddling with this parameter may fix models that appear broken.
	 */
	void make_game_object(
		gpu::ShaderPipeline* pipeline,
		gpu::RenderDevice* gpu,
		gengine::PhysicsEngine* physics_engine,
		gengine::TextureFactory* texture_factory,
		const glm::mat4& matrix,
		gengine::Collidable* rigidbody,
		std::string_view model_path,
		bool flipUVs = false,
		bool flipWindingOrder = false);

	void add_game_object(const glm::mat4& matrix, TactileCapsule&&, VisualModel&&);

	void add_game_object(const glm::mat4& matrix, TactileSphere&&, VisualModel&&);

	void add_game_object(const glm::mat4& matrix, VisualModel&&);

	std::unique_ptr<Scene> build(
		gpu::ShaderPipeline* pipeline,
		gpu::RenderDevice* gpu,
		gengine::PhysicsEngine* physics_engine,
		gengine::TextureFactory* texture_factory);
};
