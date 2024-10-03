#pragma once

#include "physics.h"
#include "gpu.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

struct Scene {
	std::vector<glm::mat4> transforms{};
	std::vector<gengine::Collidable*> collidables{};
	std::vector<gpu::Geometry*> render_components{};
	std::vector<gpu::Descriptors*> descriptors{};
};

class SceneBuilder {
	std::unique_ptr<Scene> scene;

	gengine::TextureFactory* texture_factory;
	gengine::PhysicsEngine* physics_engine;
	gpu::RenderDevice* gpu;

public:
	SceneBuilder(
		gpu::RenderDevice* gpu,
		gengine::PhysicsEngine* physics_engine,
		gengine::TextureFactory* texture_factory);
	~SceneBuilder();

	void add_game_object(
		gpu::ShaderPipeline* pipeline,
		const glm::mat4& matrix,
		gengine::Collidable* rigidbody,
		std::string_view model_path,
		bool makeMesh = false,
		bool flipUVs = false,
		bool flipWindingOrder = false);

	std::unique_ptr<Scene> get_scene();
};
