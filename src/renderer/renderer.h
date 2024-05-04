#pragma once

#include <glm/glm.hpp>

#include <string_view>
#include <vector>

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

struct ImageInfo {
	unsigned int width;
	unsigned int height;
	unsigned int channel_count;
};

struct Renderable {
	gengine::Buffer* vbo;
	gengine::Buffer* ebo;
	unsigned long index_count;
};

class RenderDevice {
public:
	// device management

	static auto create(GLFWwindow* window) -> RenderDevice*;

	static auto destroy(RenderDevice* device) -> void;

	// resource managemnet

	virtual auto create_buffer(const BufferInfo& info, const void* data) -> Buffer* = 0;

	virtual auto destroy_buffer(Buffer* buffer) -> void = 0;

	virtual auto create_image(const ImageInfo& info, const void* data) -> Image* = 0;

	virtual auto destroy_image(Image* image) -> void = 0;

	virtual auto create_pipeline(const std::string_view vert_code, const std::string_view frag_code)
		-> ShaderPipeline* = 0;

	virtual auto create_descriptors(ShaderPipeline* pipeline, Image* albedo, const glm::vec3& color) -> Descriptors* = 0;

	virtual auto destroy_pipeline(ShaderPipeline* pso) -> void = 0;

	virtual auto create_renderable(
		const std::vector<float>& vertices,
		const std::vector<float>& vertices_aux,
		const std::vector<uint32_t> indices) -> Renderable = 0;

	virtual auto render(
		const glm::mat4& view,
		ShaderPipeline* pipeline,
		const std::vector<glm::mat4>& transforms,
		const std::vector<Renderable>& renderables,
		const std::vector<Descriptors*>& descriptors) -> void = 0;
};
} // namespace gengine
