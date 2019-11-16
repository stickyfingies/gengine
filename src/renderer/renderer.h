#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <string_view>

struct GLFWwindow;

namespace gengine
{
	struct ShaderModule;
	struct ShaderPipeline;
	struct VertexBuffer;

	struct RenderImage;

	class RenderCmdList
	{
	public:

		virtual auto start_recording()->void = 0;

		virtual auto stop_recording()->void = 0;

		virtual auto start_frame(ShaderPipeline* pso)->void = 0;

		virtual auto end_frame()->void = 0;

		virtual auto bind_pipeline(ShaderPipeline* pso)->void = 0;

		virtual auto bind_vertex_buffer(VertexBuffer* vbo)->void = 0;

		// very hacky temporary solution, just like the rest of the codebase
		virtual auto push_constants(gengine::ShaderPipeline* pso, const glm::mat4 transform, const float* view)->void = 0;

		virtual auto draw(int vertex_count, int instance_count)->void = 0;
	};

	class Renderer
	{
	public:

		virtual auto create_vertex_buffer(const std::vector<float>& vertices, const std::vector<unsigned int>& indices)->VertexBuffer* = 0;

		virtual auto destroy_vertex_buffer(VertexBuffer* buffer)->void = 0;

		virtual auto create_shader_module(const std::string_view code)->ShaderModule* = 0;

		virtual auto destroy_shader_module(ShaderModule* module)->void = 0;

		virtual auto create_pipeline(ShaderModule* vert, ShaderModule* frag)->ShaderPipeline* = 0;

		virtual auto destroy_pipeline(ShaderPipeline* pso)->void = 0;

		virtual auto get_swapchain_image()->RenderImage* = 0;

		virtual auto alloc_cmdlist()->RenderCmdList* = 0;

		virtual auto execute_cmdlist(RenderCmdList* cmdlist)->void = 0;

		virtual auto free_cmdlist(RenderCmdList* cmdlist)->void = 0;
	};

	auto create_renderer(GLFWwindow* window)->Renderer*;

	auto destroy_renderer(Renderer* renderer)->void;
}