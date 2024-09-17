#pragma once

#include <glm/glm.hpp>

#include "../assets.h"

#include <string_view>
#include <vector>
#include <functional>
#include <memory>

struct GLFWwindow;

namespace gengine {
	
struct ShaderPipeline;
struct Descriptors;
struct Buffer;
struct Image;
struct RenderImage;

struct BufferInfo {
	enum class Usage { VERTEX, INDEX };

	Usage usage;
	size_t stride;
	size_t element_count;
};

struct Renderable {
	std::shared_ptr<Buffer> vbo;
	std::shared_ptr<Buffer> ebo;
	unsigned long index_count;
};

class RenderDevice {
public:
	// device management

	static auto create(GLFWwindow* window) -> std::unique_ptr<RenderDevice>;

	// resource managemnet

	virtual auto create_buffer(const BufferInfo& info, const void* data) -> std::unique_ptr<Buffer> = 0;

	/// TODO - This is undesirable.
	///
	/// - destroy_buffer completely invalidates the usage of that buffer.
	/// - shared_ptr indicates the buffer may still be in use.
	///
	/// These are conflicting goals.
	virtual auto destroy_buffer(std::shared_ptr<Buffer> buffer) -> void = 0;

	virtual auto create_image(const ImageAsset& image_asset) -> Image* = 0;

	virtual auto destroy_all_images() -> void = 0;

	virtual auto create_pipeline(const std::string_view vert_code, const std::string_view frag_code)
		-> ShaderPipeline* = 0;

	virtual auto create_descriptors(ShaderPipeline* pipeline, Image* albedo, const glm::vec3& color) -> Descriptors* = 0;

	virtual auto destroy_pipeline(ShaderPipeline* pso) -> void = 0;

	virtual auto create_renderable(const GeometryAsset& geometry) -> Renderable = 0;

	virtual auto render(
		const glm::mat4& view,
		ShaderPipeline* pipeline,
		const std::vector<glm::mat4>& transforms,
		const std::vector<Renderable>& renderables,
		const std::vector<Descriptors*>& descriptors,
		std::function<void()> gui_code) -> void = 0;
};
} // namespace gengine
