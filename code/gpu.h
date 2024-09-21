#pragma once

#include <glm/glm.hpp>

#include "assets.h"

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

struct GLFWwindow;

namespace gpu {

struct ShaderPipeline;
struct Descriptors;
struct Buffer;
struct Image;
struct RenderImage;
struct Geometry;

struct BufferInfo {
	enum class Usage { VERTEX, INDEX };

	Usage usage;
	size_t stride;
	size_t element_count;
};

class RenderDevice {
public:
	// device management

	static auto create(GLFWwindow* window) -> std::unique_ptr<RenderDevice>;

	static auto configure_glfw() -> void;

	virtual ~RenderDevice() = default;

	// resource managemnet

	virtual auto
	create_buffer(const BufferInfo& info, const void* data) -> std::unique_ptr<Buffer> = 0;

	/// TODO 2024-09-18 - I still haven't done this?
	/// TODO 2024-05-13 - This is undesirable.
	/// TODO - destroy_buffer completely invalidates the usage of that buffer.
	/// TODO - shared_ptr indicates the buffer may still be in use.
	/// TODO - These are conflicting goals.
	virtual auto destroy_buffer(std::shared_ptr<Buffer> buffer) -> void = 0;

	virtual auto create_image(const gengine::ImageAsset& image_asset) -> Image* = 0;

	virtual auto destroy_all_images() -> void = 0;

	virtual auto create_pipeline(const std::string_view vert_code, const std::string_view frag_code)
		-> ShaderPipeline* = 0;

	virtual auto create_descriptors(ShaderPipeline* pipeline, Image* albedo, const glm::vec3& color)
		-> Descriptors* = 0;

	virtual auto destroy_pipeline(ShaderPipeline* pso) -> void = 0;

	virtual auto create_geometry(const gengine::GeometryAsset& geometry) -> Geometry* = 0;

	virtual auto destroy_geometry(const Geometry* geometry) -> void = 0;

	virtual auto render(
		const glm::mat4& view,
		ShaderPipeline* pipeline,
		const std::vector<glm::mat4>& transforms,
		const std::vector<Geometry*>& renderables,
		const std::vector<Descriptors*>& descriptors,
		std::function<void()> gui_code) -> void = 0;
};
} // namespace gpu
