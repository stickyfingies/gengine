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
struct ShaderPipeline;
struct Descriptors;
struct Buffer;
struct Image;
struct RenderImage;
struct Geometry;

/**
 * Describes the shape and usage of a VRAM allocation
 */
struct BufferInfo {
	enum class Usage { VERTEX, INDEX };
	Usage usage;
	std::size_t stride;
	std::size_t element_count;
};

/**
 * NOTICE: This MUST run before `glfwCreateWindow` and after `glfwInit`.
 */
auto configure_glfw() -> void;

/**
 * @class gpu::RenderDevice
 * A physical hardware accelerator.
 */
class RenderDevice {
public:
	/**
	 * Connect a hardware accelerator to the windowing system.
	 * @param window The OS window on which to draw graphics.
	 */
	static auto create(GLFWwindow* window) -> std::unique_ptr<RenderDevice>;

	/**
	 * @brief The destructor is NOT RESPONSIBLE for destroying GPU resources.
	 *
	 * NOTICE: Be careful to free GPU resources before it is destroyed.
	 */
	virtual ~RenderDevice() = default;

	/**
	 * Allocate VRAM and instantiate it with data from RAM.
	 * @param info describes the VRAM allocation shape and its usage
	 * @param data is a region of RAM which is uploaded over PCI-e
	 */
	virtual auto
	create_buffer(const BufferInfo& info, const void* data) -> std::unique_ptr<Buffer> = 0;
	
	/// TODO 2024-09-18 - I still haven't done this?
	/// TODO 2024-05-13 - This is undesirable.
	/// TODO - destroy_buffer completely invalidates the usage of that buffer.
	/// TODO - shared_ptr indicates the buffer may still be in use.
	/// TODO - These are conflicting goals.
	/**
	 * Free a region of VRAM.
	 * @param buffer VRAM to free
	 */
	virtual auto destroy_buffer(std::shared_ptr<Buffer> buffer) -> void = 0;

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
	 * @param vert_code 
	 * @param frag_code 
	 * @return ShaderPipeline* 
	 */
	virtual auto create_pipeline(const std::string_view vert_code, const std::string_view frag_code)
		-> ShaderPipeline* = 0;

	/**
	 * "Descriptors" bind GPU resources to slots in shaders.
	 * @param pipeline a raster pipeline
	 * @param albedo a texture to apply
	 * @param color rgb color to apply
	 */
	virtual auto create_descriptors(ShaderPipeline* pipeline, Image* albedo, const glm::vec3& color)
		-> Descriptors* = 0;

	virtual auto destroy_pipeline(ShaderPipeline* pso) -> void = 0;

	/**
	 * Convert raw geometry data into a GPU geometry object.
	 * @param vertices see implementation
	 * @param vertices_aux see implementation
	 * @param indices see implementation
	 */
	virtual auto create_geometry(
		const std::vector<float>& vertices,
		const std::vector<float>& vertices_aux,
		const std::vector<unsigned int>& indices) -> Geometry* = 0;

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
