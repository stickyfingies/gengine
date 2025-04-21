/**
 * @headerfile gpu.h
 * @brief An abstract GPU interface for cross-platform rendering.
 *
 * This file describes a low-level GPU interface that is designed to
 * abstract over platform-specifc details like:
 * - the structure of the raster pipeline
 * - VRAM dynamic memory operations
 * - geometric encodings of 3D shapes
 *
 * @author Seth Traman <github.com/stickyfingies>
 * @copyright GPLv3 (https://www.gnu.org/licenses/gpl-3.0.en.html)
 *
 * TODO(Seth) - concept: "meshes" describe how to use "geometry buffers" inside "render pipelines".
 * TODO(Seth) - vertex structure is described when creating render pipelines.
 */

#pragma once

#include <glm/glm.hpp>

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

struct GLFWwindow;

namespace gpu {

// GPU Resources
// TODO(Seth) - These are slowly moving into the private interface because
// I'm moving from pointers to handles for GPU resources, which I want to be
// strongly & uniquely typed but are ultimately just integers.
struct ShaderPipeline;
struct Descriptors;
struct Buffer;
struct Image;
struct RenderImage;
struct Geometry;

/**
 * A "vertex" is a set of attributes, like position, texture coordinates, etc.
 */
enum class VertexAttribute { VEC3_FLOAT, VEC2_FLOAT };

enum class BufferUsage { VERTEX, INDEX };

/**
 * NOTICE: This MUST run before `glfwCreateWindow` and after `glfwInit`.
 */
auto configure_glfw() -> void;

struct BufferHandle {
	uint64_t id;
};

struct ShaderPipelineHandle {
	uint64_t id;
};

/**
 * @class gpu::RenderDevice
 * A physical hardware accelerator.
 */
class RenderDevice {
public:
	/**
	 * Connect a hardware accelerator to the windowing system.
	 * Make sure to call `gpu::configure_glfw()` before this.
	 * @param window The OS window on which to draw graphics.
	 */
	static auto create(std::shared_ptr<GLFWwindow> window) -> std::unique_ptr<RenderDevice>;

	/**
	 * @brief The destructor is NOT RESPONSIBLE for destroying GPU resources.
	 * @note  This should improve after I switch the interface from PIMPL to handles.
	 *
	 * NOTICE: Be careful to free GPU resources before it is destroyed.
	 */
	virtual ~RenderDevice() = default;

	/**
	 * Allocate VRAM and instantiate it with data from RAM.
	 */
	virtual auto create_buffer(
		BufferUsage usage, std::size_t stride, std::size_t element_count, const void* data)
		-> BufferHandle = 0;

	/// TODO 2024-09-18 - I still haven't done this?
	/// TODO 2024-05-13 - This is undesirable.
	/// TODO - destroy_buffer completely invalidates the usage of that buffer.
	/// TODO - shared_ptr indicates the buffer may still be in use.
	/// TODO - These are conflicting goals.
	/**
	 * Free a region of VRAM.
	 * @param buffer VRAM to free
	 */
	virtual auto destroy_buffer(BufferHandle buffer) -> void = 0;

	/**
	 * @brief Allocate VRAM and instantiate it with a bitmap image.
	 * @param name unique
	 * @param width pixels
	 * @param height pixels
	 * @param channel_count RGB=3 and RGBA=4
	 * @param data size must be width*height*channel_count
	 */
	virtual auto create_image(
		const std::string& name, int width, int height, int channel_count, unsigned char* data)
		-> Image* = 0;

	/**
	 * @deprecated may be removed in the future
	 */
	virtual auto destroy_all_images() -> void = 0;

	/**
	 * Construct a raster pipeline which contains a shader program
	 * @param vert_code vertex shader content
	 * @param frag_code fragment shader content
	 * @param vertex_attributes a list of attributes used in each vertex
	 * @return ShaderPipelineHandle
	 */
	virtual auto create_pipeline(
		std::string_view vert_code,
		std::string_view frag_code,
		const std::vector<VertexAttribute>& vertex_attributes) -> ShaderPipelineHandle = 0;

	/**
	 * "Descriptors" bind GPU resources to slots in shaders.
	 * @param pipeline a raster pipeline
	 * @param albedo a texture to apply
	 * @param color rgb color to apply
	 */
	virtual auto
	create_descriptors(ShaderPipelineHandle pipeline, Image* albedo, const glm::vec3& color)
		-> Descriptors* = 0;

	virtual auto destroy_pipeline(ShaderPipelineHandle pso) -> void = 0;

	/**
	 * Convert raw geometry data into a GPU geometry object.
	 * @param vertices see implementation
	 * @param vertices_aux see implementation
	 * @param indices see implementation
	 */
	virtual auto create_geometry(
		ShaderPipelineHandle pipeline, BufferHandle vertex_buffer, BufferHandle index_buffer)
		-> Geometry* = 0;

	virtual auto destroy_geometry(const Geometry* geometry) -> void = 0;

	virtual auto render(
		const glm::mat4& view,
		ShaderPipelineHandle pipeline,
		const std::vector<glm::mat4>& transforms,
		const std::vector<Geometry*>& renderables,
		const std::vector<Descriptors*>& descriptors,
		std::function<void()> gui_code) -> void = 0;
};
} // namespace gpu
