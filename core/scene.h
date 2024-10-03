#pragma once

#include "physics.h"
#include "gpu.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

/**
 * @brief \c Scene is created and used by Scene Adapters.
 */
struct Scene {
	std::vector<glm::mat4> transforms{};
	std::vector<gengine::Collidable*> collidables{};
	std::vector<gpu::Geometry*> render_components{};
	std::vector<gpu::Descriptors*> descriptors{};
};

/**
 * @brief \c SceneBuilder is an interface for describing and creating a \c Scene.
 */
class SceneBuilder {

	/**
	 * The scene to build
	 */
	std::unique_ptr<Scene> scene;

	/**
	 * Used to load game assets.
	 */
	gengine::TextureFactory* texture_factory;

	/**
	 * Used to create rigidbodies from 3D models.
	 */
	gengine::PhysicsEngine* physics_engine;

	/**
	 * Used to create GPU rendering data.
	 */
	gpu::RenderDevice* gpu;

public:
	SceneBuilder(
		gpu::RenderDevice* gpu,
		gengine::PhysicsEngine* physics_engine,
		gengine::TextureFactory* texture_factory);
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
	void add_game_object(
		gpu::ShaderPipeline* pipeline,
		const glm::mat4& matrix,
		gengine::Collidable* rigidbody,
		std::string_view model_path,
		bool flipUVs = false,
		bool flipWindingOrder = false);

	/**
	 * @brief Retrieve the finished scene.  Invalidates this SceneBuilder from further use.
	 * 
	 * @return the \c Scene built so far.
	 */
	std::unique_ptr<Scene> finish();
};
