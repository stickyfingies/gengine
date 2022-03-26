#pragma once

#include <glm/glm.hpp>

#include <string_view>

struct GLFWwindow;

namespace gengine {
struct ShaderPipeline;
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

class RenderContext {
public:
	virtual auto begin() -> void = 0;

	virtual auto end() -> void = 0;

	virtual auto bind_pipeline(ShaderPipeline *pso) -> void = 0;

	virtual auto bind_geometry_buffers(Buffer *vbo, Buffer *ebo) -> void = 0;

	// very hacky temporary solution, just like the rest of the codebase
	virtual auto push_constants(gengine::ShaderPipeline *pso, const glm::mat4 transform, const float *view) -> void = 0;

	virtual auto draw(int vertex_count, int instance_count) -> void = 0;
};

class RenderDevice {
public:
	// device management

	static auto create(GLFWwindow *window) -> RenderDevice *;

	static auto destroy(RenderDevice *device) -> void;

	// resource managemnet

	virtual auto create_buffer(const BufferInfo &info, const void *data) -> Buffer * = 0;

	virtual auto destroy_buffer(Buffer *buffer) -> void = 0;

	virtual auto create_image(const ImageInfo &info, const void *data) -> Image * = 0;

	virtual auto destroy_image(Image *image) -> void = 0;

	virtual auto create_pipeline(const std::string_view vert_code, const std::string_view frag_code, Image *albedo)
		-> ShaderPipeline * = 0;

	virtual auto destroy_pipeline(ShaderPipeline *pso) -> void = 0;

	// swapchain control

	virtual auto get_swapchain_image() -> Image * = 0;

	// gpu workload management

	virtual auto alloc_context() -> RenderContext * = 0;

	virtual auto execute_context(RenderContext *cmdlist) -> void = 0;

	virtual auto free_context(RenderContext *cmdlist) -> void = 0;
};
} // namespace gengine
