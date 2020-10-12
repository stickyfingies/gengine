#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <string_view>

struct GLFWwindow;

namespace gengine
{
	struct ShaderPipeline;
	struct Buffer;
	struct RenderImage;

	struct BufferInfo
	{
		enum class Usage
		{
			VERTEX,
			INDEX
		};

		Usage usage;
		unsigned long stride;
		unsigned long element_count;
	};

	struct RenderComponent
	{
		Buffer* vbo;
		Buffer* ebo;
		unsigned int index_count;
	};

	class RenderContext
	{
	public:

		virtual auto begin()->void = 0;

		virtual auto end()->void = 0;

		virtual auto bind_pipeline(ShaderPipeline* pso)->void = 0;

		virtual auto bind_geometry_buffers(Buffer* vbo, Buffer* ebo)->void = 0;

		// very hacky temporary solution, just like the rest of the codebase
		virtual auto push_constants(gengine::ShaderPipeline* pso, const glm::mat4 transform, const float* view)->void = 0;

		virtual auto draw(int vertex_count, int instance_count)->void = 0;
	};

	/**
	 * @brief Represents a low-level rendering construct which maps to a physical GPU
	 */
	class RenderDevice
	{
	public:
	
		/**
		 * @brief Create a GPU buffer object
		 * 
		 * @param info Creation info describing the buffer to be created
		 * @param data Data to be filled into the buffer

		 * @return Buffer* 
		 */
		virtual auto create_buffer(const BufferInfo& info, const void* data)->Buffer* = 0;

		/**
		 * @brief Destroy a GPU buffer object
		 * 
		 * @param buffer Buffer to be destroyed
		 */
		virtual auto destroy_buffer(Buffer* buffer)->void = 0;

		/**
		 * @brief Create a pipeline object
		 * 
		 * @param vert raw SPIR-V vertex shader code
		 * @param frag raw SPIR-V fragment shader code
		 
		 * @return ShaderPipeline* 
		 */
		virtual auto create_pipeline(const std::string_view vert, const std::string_view frag)->ShaderPipeline* = 0;
		
		/**
		 * @brief Destroy a pipeline object
		 * 
		 * @param pso pipeline to be destroyed
		 */
		virtual auto destroy_pipeline(ShaderPipeline* pso)-> void = 0;

		/**
		 * @brief Get the swapchain image object for the current frame
		 * 
		 * @return RenderImage* 
		 */
		virtual auto get_swapchain_image()->RenderImage* = 0;

		/**
		 * @brief Allocate a render context attached to the current frame's backbuffer
		 *
		 * @return RenderContext* 
		 */
		virtual auto alloc_context()->RenderContext* = 0;

		/**
		 * @brief Execute the GPU instructions recorded into a context
		 * 
		 * @param cmdlist context to be executed
		 */
		virtual auto execute_context(RenderContext* cmdlist)->void = 0;
		
		/**
		 * @brief De-allocate a render context
		 * 
		 * @param cmdlist context to be de-allocated
		 */
		virtual auto free_context(RenderContext* cmdlist)->void = 0;
	};

	auto init_renderer(bool debug)->void;

	auto create_render_device(GLFWwindow* window)->RenderDevice*;

	auto destroy_render_device(RenderDevice* device)->void;
}
