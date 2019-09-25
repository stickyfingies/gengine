#include "renderer.h"

#include "vulkan-headers.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <vector>

#undef min
#undef max

static const auto FRAMES_IN_FLIGHT = 2;
static const auto SWAPCHAIN_SIZE = 3;

static auto load_file(const char* path)->std::vector<char>
{
	std::ifstream file(path, std::ios::ate | std::ios::binary);

	if (file.is_open() == false)
	{
		std::cout << "[app] - err :: Failed to open file" << std::endl;
		return {};
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

static auto find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties)->uint32_t
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

	for (auto i = 0; i < mem_properties.memoryTypeCount; ++i)
	{
		if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	std::cout << "Failed to find suitable memory type" << std::endl;
	return 0;
}

struct Vertex
{
	glm::vec3 pos;

	static auto get_binding_desc()->vk::VertexInputBindingDescription
	{
		const vk::VertexInputBindingDescription binding_desc(0, sizeof(Vertex), vk::VertexInputRate::eVertex);

		return binding_desc;
	}

	static auto get_attribute_descs()->std::array<vk::VertexInputAttributeDescription, 1>
	{
		const std::array<vk::VertexInputAttributeDescription, 1> attribute_descs
		{
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos))
		};

		return attribute_descs;
	}
};

struct PushConstantData
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

namespace gengine
{
	struct Renderer::Impl
	{
		vk::Instance instance;

		VkSurfaceKHR surface;

		vk::SwapchainKHR swapchain;

		vk::Format surface_fmt;

		vk::PresentModeKHR present_mode;

		vk::Extent2D extent;

		vk::PhysicalDevice physical_device;

		vk::Device device;

		vk::RenderPass backbuffer_pass;

		vk::PipelineLayout pipeline_layout;
		vk::Pipeline pipeline;

		vk::Buffer vertex_buffer;

		vk::DeviceMemory vertex_buffer_mem;

		vk::Queue graphics_queue;
		vk::Queue present_queue;

		int graphics_queue_idx;
		int present_queue_idx;

		std::vector<vk::Semaphore> image_available_semaphores;
		std::vector<vk::Semaphore> render_finished_semaphores;
		std::vector<vk::Fence> swapchain_fences;
		std::vector<vk::Image> swapchain_images;
		std::vector<vk::ImageView> swapchain_views;
		std::vector<vk::Framebuffer> backbuffers;
		std::vector<vk::CommandPool> cmd_pools;
		std::vector<vk::CommandBuffer> cmd_buffers;

		unsigned short current_frame = 0;
		unsigned int image_idx;
	};

	Renderer::Renderer(GLFWwindow* window)
		: impl{ new Impl }
	{
		// create instance
		{
			vk::ApplicationInfo const app_info("App Name", VK_MAKE_VERSION(1, 0, 0), "Engine Name", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0);

			std::array<char const*, 3> const extension_names =
			{
				VK_KHR_SURFACE_EXTENSION_NAME,
				#ifdef WIN32
				VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
				#else
				VK_KHR_XCB_SURFACE_EXTENSION_NAME,
				#endif
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME
			};

			std::array<char const*, 1> const layer_names = { "VK_LAYER_KHRONOS_validation" };

			vk::InstanceCreateInfo const instance_info(vk::InstanceCreateFlags{}, &app_info, layer_names.size(), layer_names.data(), extension_names.size(), extension_names.data());

			impl->instance = vk::createInstance(instance_info);
		}

		// create surface
		{
			glfwCreateWindowSurface(impl->instance, window, nullptr, &impl->surface);
		}

		// choose physical device
		{
			auto const physical_devices = impl->instance.enumeratePhysicalDevices();

			impl->physical_device = physical_devices.front();
		}

		// create logical device
		{
			auto const queue_family_properties = impl->physical_device.getQueueFamilyProperties();

			impl->graphics_queue_idx = std::distance(
				queue_family_properties.begin(),
				std::find_if(
					queue_family_properties.begin(),
					queue_family_properties.end(),
					[](vk::QueueFamilyProperties const& qfp)
					{
						return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
					}
				)
			);

			impl->present_queue_idx = impl->physical_device.getSurfaceSupportKHR(impl->graphics_queue_idx, impl->surface) ? impl->graphics_queue_idx : queue_family_properties.size();

			if (impl->present_queue_idx == queue_family_properties.size())
			{
				for (auto i = 0; i < queue_family_properties.size(); ++i)
				{
					if ((queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) && impl->physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(i), impl->surface))
					{
						impl->graphics_queue_idx = i;
						impl->present_queue_idx = i;
						break;
					}
				}
				if (impl->present_queue_idx == queue_family_properties.size())
				{
					for (auto i = 0; i < queue_family_properties.size(); ++i)
					{
						if (impl->physical_device.getSurfaceSupportKHR(i, impl->surface))
						{
							impl->present_queue_idx = i;
							break;
						}
					}
				}
			}

			std::array<float, 1> const queue_priorities = { 1.0f };

			vk::DeviceQueueCreateInfo const device_queue_create_info(vk::DeviceQueueCreateFlags(), impl->graphics_queue_idx, 1, queue_priorities.data());

			std::array<char const*, 1> const extension_names = { "VK_KHR_swapchain" };

			vk::DeviceCreateInfo const device_info(vk::DeviceCreateFlags(), 1, &device_queue_create_info, 0, nullptr, extension_names.size(), extension_names.data());

			impl->device = impl->physical_device.createDevice(device_info);

			impl->graphics_queue = impl->device.getQueue(impl->graphics_queue_idx, 0);
			impl->present_queue = impl->device.getQueue(impl->present_queue_idx, 0);
		}

		// create command pool / buffers
		{
			for (auto i = 0; i < FRAMES_IN_FLIGHT; ++i)
			{
				vk::CommandPoolCreateInfo const pool_info(vk::CommandPoolCreateFlags(), impl->graphics_queue_idx);

				impl->cmd_pools.push_back(impl->device.createCommandPool(pool_info));

				vk::CommandBufferAllocateInfo const alloc_info(impl->cmd_pools[i], vk::CommandBufferLevel::ePrimary, 1);

				impl->cmd_buffers.push_back(impl->device.allocateCommandBuffers(alloc_info).at(0));
			}
		}

		// create swapchain
		{
			int width;
			int height;

			glfwGetWindowSize(window, &width, &height);

			const auto formats = impl->physical_device.getSurfaceFormatsKHR(impl->surface);

			impl->surface_fmt = (formats[0].format == vk::Format::eUndefined) ? vk::Format::eB8G8R8A8Unorm : formats[0].format;

			const auto surface_caps = impl->physical_device.getSurfaceCapabilitiesKHR(impl->surface);

			if (surface_caps.currentExtent.width == std::numeric_limits<uint32_t>::max())
			{
				impl->extent.width = std::clamp(static_cast<uint32_t>(width), surface_caps.minImageExtent.width, surface_caps.maxImageExtent.width);
				impl->extent.height = std::clamp(static_cast<uint32_t>(height), surface_caps.minImageExtent.height, surface_caps.maxImageExtent.height);
			}
			else
			{
				impl->extent = surface_caps.currentExtent;
			}

			const auto present_modes = impl->physical_device.getSurfacePresentModesKHR(impl->surface);

			impl->present_mode = vk::PresentModeKHR::eFifo;

			for (const auto& present_mode : present_modes)
			{
				if (present_mode == vk::PresentModeKHR::eMailbox)
				{
					impl->present_mode = present_mode;
				}
				else if (present_mode == vk::PresentModeKHR::eImmediate)
				{
					impl->present_mode = present_mode;
				}
			}

			const auto pre_transform = (surface_caps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ? vk::SurfaceTransformFlagBitsKHR::eIdentity : surface_caps.currentTransform;

			const auto composite_alpha =
				(surface_caps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied :
				(surface_caps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied :
				(surface_caps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ? vk::CompositeAlphaFlagBitsKHR::eInherit : vk::CompositeAlphaFlagBitsKHR::eOpaque;

			vk::SwapchainCreateInfoKHR swapchain_info(vk::SwapchainCreateFlagsKHR(), impl->surface, surface_caps.minImageCount, impl->surface_fmt, vk::ColorSpaceKHR::eSrgbNonlinear,
				impl->extent, 1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0, nullptr, pre_transform, composite_alpha, impl->present_mode, true, nullptr);

			const std::array<uint32_t, 2> queue_family_indices = { impl->graphics_queue_idx, impl->present_queue_idx };

			if (impl->graphics_queue_idx != impl->present_queue_idx)
			{
				swapchain_info.imageSharingMode = vk::SharingMode::eConcurrent;
				swapchain_info.queueFamilyIndexCount = queue_family_indices.size();
				swapchain_info.pQueueFamilyIndices = queue_family_indices.data();
			}

			impl->swapchain = impl->device.createSwapchainKHR(swapchain_info);

			impl->swapchain_images = impl->device.getSwapchainImagesKHR(impl->swapchain);

			const vk::ComponentMapping component_mapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
			const vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

			for (const auto& image : impl->swapchain_images)
			{
				const vk::ImageViewCreateInfo view_info(vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, impl->surface_fmt, component_mapping, subresource_range);

				impl->swapchain_views.push_back(impl->device.createImageView(view_info));
			}

			// Create semaphores

			for (auto i = 0; i < FRAMES_IN_FLIGHT; ++i)
			{
				impl->image_available_semaphores.push_back(impl->device.createSemaphore(vk::SemaphoreCreateInfo()));
				impl->render_finished_semaphores.push_back(impl->device.createSemaphore(vk::SemaphoreCreateInfo()));

				impl->swapchain_fences.push_back(impl->device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)));
			}
		}

		// create render pass
		{
			const vk::AttachmentDescription backbuffer_desc(vk::AttachmentDescriptionFlags(), impl->surface_fmt, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);

			const std::array<vk::AttachmentDescription, 1> attachments = { backbuffer_desc };

			const vk::AttachmentReference backbuffer_ref(0, vk::ImageLayout::eColorAttachmentOptimal);

			const std::array<vk::AttachmentReference, 1> attachment_refs = { backbuffer_ref };

			const vk::SubpassDescription fwd_subpass(vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, 0, nullptr, attachment_refs.size(), attachment_refs.data());

			const std::array<vk::SubpassDescription, 1> subpasses = { fwd_subpass };

			const vk::RenderPassCreateInfo render_pass_info(vk::RenderPassCreateFlags(), attachments.size(), attachments.data(), subpasses.size(), subpasses.data());

			impl->backbuffer_pass = impl->device.createRenderPass(render_pass_info);
		}

		// create framebuffers
		{
			for (auto i = 0; i < impl->swapchain_views.size(); ++i)
			{
				const std::array<vk::ImageView, 1> attachments = { impl->swapchain_views[i] };

				vk::FramebufferCreateInfo framebuffer_info(vk::FramebufferCreateFlags(), impl->backbuffer_pass, attachments.size(), attachments.data(), impl->extent.width, impl->extent.height, 1);

				impl->backbuffers.push_back(impl->device.createFramebuffer(framebuffer_info));
			}
		}

		// create pipeline
		{
			const vk::PushConstantRange push_const_range(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantData));

			const std::array<vk::PushConstantRange, 1> push_const_ranges = { push_const_range };

			const vk::PipelineLayoutCreateInfo layout_info(vk::PipelineLayoutCreateFlags(), 0, nullptr, push_const_ranges.size(), push_const_ranges.data());

			impl->pipeline_layout = impl->device.createPipelineLayout(layout_info);

			const auto vert_shader_code = load_file("../../data/cube.vert.spv");
			const auto frag_shader_code = load_file("../../data/cube.frag.spv");

			const vk::ShaderModuleCreateInfo vert_module_info(vk::ShaderModuleCreateFlags(), vert_shader_code.size(), reinterpret_cast<const uint32_t*>(vert_shader_code.data()));
			const vk::ShaderModuleCreateInfo frag_module_info(vk::ShaderModuleCreateFlags(), frag_shader_code.size(), reinterpret_cast<const uint32_t*>(frag_shader_code.data()));

			const auto vert_shader_module = impl->device.createShaderModule(vert_module_info);
			const auto frag_shader_module = impl->device.createShaderModule(frag_module_info);

			const vk::PipelineShaderStageCreateInfo vert_shader_info(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, vert_shader_module, "main");
			const vk::PipelineShaderStageCreateInfo frag_shader_info(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, frag_shader_module, "main");

			const std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages = { vert_shader_info, frag_shader_info };

			const auto binding_desc = Vertex::get_binding_desc();
			const auto attribute_descs = Vertex::get_attribute_descs();

			const vk::PipelineVertexInputStateCreateInfo vertex_input_info(vk::PipelineVertexInputStateCreateFlags(), 1, &binding_desc, attribute_descs.size(), attribute_descs.data());

			const vk::PipelineInputAssemblyStateCreateInfo input_assembly(vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList, VK_FALSE);

			const vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(impl->extent.width), static_cast<float>(impl->extent.height), 0.0f, 1.0f);

			const vk::Rect2D scissor(vk::Offset2D(), impl->extent);

			const vk::PipelineViewportStateCreateInfo viewport_state(vk::PipelineViewportStateCreateFlags(), 1, &viewport, 1, &scissor);

			const vk::PipelineRasterizationStateCreateInfo rasterizer(vk::PipelineRasterizationStateCreateFlags(), VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eFront, vk::FrontFace::eCounterClockwise, VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f);

			const vk::PipelineMultisampleStateCreateInfo multisampling(vk::PipelineMultisampleStateCreateFlags(), vk::SampleCountFlagBits::e1);

			const std::array<vk::PipelineColorBlendAttachmentState, 1> color_blend_attachments
			{
				vk::PipelineColorBlendAttachmentState(VK_FALSE, vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
			};

			const vk::PipelineColorBlendStateCreateInfo color_blending(vk::PipelineColorBlendStateCreateFlags(), VK_FALSE, vk::LogicOp::eCopy, color_blend_attachments.size(), color_blend_attachments.data(), { (0.0f, 0.0f, 0.0f, 0.0f) });

			const vk::GraphicsPipelineCreateInfo pipeline_info
			(
				vk::PipelineCreateFlags(),
				shader_stages.size(),
				shader_stages.data(),
				&vertex_input_info,
				&input_assembly,
				nullptr,
				&viewport_state,
				&rasterizer,
				&multisampling,
				nullptr,
				&color_blending,
				nullptr,
				impl->pipeline_layout,
				impl->backbuffer_pass
			);

			impl->pipeline = impl->device.createGraphicsPipeline(nullptr, pipeline_info);

			impl->device.destroyShaderModule(vert_shader_module);
			impl->device.destroyShaderModule(frag_shader_module);
		}

		// create vertex buffer
		{
			float vertices[]
			{
				-1.0f,-1.0f,-1.0f,  // -X side
				-1.0f,-1.0f, 1.0f,
				-1.0f, 1.0f, 1.0f,
				-1.0f, 1.0f, 1.0f,
				-1.0f, 1.0f,-1.0f,
				-1.0f,-1.0f,-1.0f,

				-1.0f,-1.0f,-1.0f,  // -Z side
				 1.0f, 1.0f,-1.0f,
				 1.0f,-1.0f,-1.0f,
				-1.0f,-1.0f,-1.0f,
				-1.0f, 1.0f,-1.0f,
				 1.0f, 1.0f,-1.0f,

				-1.0f,-1.0f,-1.0f,  // -Y side
				 1.0f,-1.0f,-1.0f,
				 1.0f,-1.0f, 1.0f,
				-1.0f,-1.0f,-1.0f,
				 1.0f,-1.0f, 1.0f,
				-1.0f,-1.0f, 1.0f,

				-1.0f, 1.0f,-1.0f,  // +Y side
				-1.0f, 1.0f, 1.0f,
				 1.0f, 1.0f, 1.0f,
				-1.0f, 1.0f,-1.0f,
				 1.0f, 1.0f, 1.0f,
				 1.0f, 1.0f,-1.0f,

				 1.0f, 1.0f,-1.0f,  // +X side
				 1.0f, 1.0f, 1.0f,
				 1.0f,-1.0f, 1.0f,
				 1.0f,-1.0f, 1.0f,
				 1.0f,-1.0f,-1.0f,
				 1.0f, 1.0f,-1.0f,

				-1.0f, 1.0f, 1.0f,  // +Z side
				-1.0f,-1.0f, 1.0f,
				 1.0f, 1.0f, 1.0f,
				-1.0f,-1.0f, 1.0f,
				 1.0f,-1.0f, 1.0f,
				 1.0f, 1.0f, 1.0f,
			};

			for (auto& vertex : vertices)
			{
				vertex *= 0.5;
			}

			const vk::BufferCreateInfo buffer_info(vk::BufferCreateFlags(), sizeof(vertices), vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive);

			impl->vertex_buffer = impl->device.createBuffer(buffer_info);

			const vk::MemoryRequirements mem_reqs = impl->device.getBufferMemoryRequirements(impl->vertex_buffer);

			const vk::MemoryAllocateInfo alloc_info(mem_reqs.size, find_memory_type(impl->physical_device, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

			impl->vertex_buffer_mem = impl->device.allocateMemory(alloc_info);

			impl->device.bindBufferMemory(impl->vertex_buffer, impl->vertex_buffer_mem, 0);

			void* data = impl->device.mapMemory(impl->vertex_buffer_mem, 0, buffer_info.size);

			memcpy(data, vertices, buffer_info.size);

			impl->device.unmapMemory(impl->vertex_buffer_mem);
		}
	}

	Renderer::~Renderer()
	{
		impl->device.waitIdle();

		for (auto i = 0; i < FRAMES_IN_FLIGHT; ++i)
		{
			impl->device.destroySemaphore(impl->image_available_semaphores[i]);
			impl->device.destroySemaphore(impl->render_finished_semaphores[i]);

			impl->device.destroyFence(impl->swapchain_fences[i]);
		}

		for (const auto& backbuffer : impl->backbuffers)
		{
			impl->device.destroyFramebuffer(backbuffer);
		}

		impl->device.destroyBuffer(impl->vertex_buffer);
		impl->device.freeMemory(impl->vertex_buffer_mem);

		impl->device.destroyPipeline(impl->pipeline);
		impl->device.destroyPipelineLayout(impl->pipeline_layout);

		impl->device.destroyRenderPass(impl->backbuffer_pass);

		for (const auto& view : impl->swapchain_views)
		{
			impl->device.destroyImageView(view);
		}

		impl->device.destroySwapchainKHR(impl->swapchain);

		for (const auto& command_pool : impl->cmd_pools)
		{
			impl->device.destroyCommandPool(command_pool);
		}

		impl->device.destroy();

		impl->instance.destroySurfaceKHR(impl->surface);

		impl->instance.destroy();
	}

	auto Renderer::start_frame()->void
	{
		impl->device.waitForFences({ impl->swapchain_fences[impl->current_frame] }, VK_TRUE, std::numeric_limits<uint64_t>::max());

		impl->device.resetFences({ impl->swapchain_fences[impl->current_frame] });

		impl->device.resetCommandPool(impl->cmd_pools[impl->current_frame], vk::CommandPoolResetFlags());

		impl->image_idx = impl->device.acquireNextImageKHR(impl->swapchain, std::numeric_limits<uint64_t>::max(), impl->image_available_semaphores[impl->current_frame], nullptr).value;

		const auto& command_buffer = impl->cmd_buffers[impl->current_frame];

		command_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlags()));

		const std::array<vk::ClearValue, 1> clear_colors = { vk::ClearValue(vk::ClearColorValue(std::array<float, 4>{ 1.0f, 0.0f, 0.0f, 1.0f })) };

		const vk::RenderPassBeginInfo pass_begin_info(impl->backbuffer_pass, impl->backbuffers[impl->image_idx], vk::Rect2D(vk::Offset2D(), impl->extent), clear_colors.size(), clear_colors.data());

		command_buffer.beginRenderPass(pass_begin_info, vk::SubpassContents::eInline);

		command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, impl->pipeline);
	}

	auto Renderer::end_frame()->void
	{
		const auto& command_buffer = impl->cmd_buffers[impl->current_frame];

		command_buffer.endRenderPass();

		command_buffer.end();

		const vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		const vk::SubmitInfo submit_info(1, &impl->image_available_semaphores[impl->current_frame], &wait_dst_stage_mask, 1, &command_buffer, 1, &impl->render_finished_semaphores[impl->current_frame]);

		impl->graphics_queue.submit({ submit_info }, impl->swapchain_fences[impl->current_frame]);

		const vk::PresentInfoKHR present_info(1, &impl->render_finished_semaphores[impl->current_frame], 1, &impl->swapchain, &impl->image_idx);

		impl->present_queue.presentKHR(present_info);

		impl->current_frame = (impl->current_frame + 1) % FRAMES_IN_FLIGHT;
	}

	auto Renderer::draw_box(float* transform)->void
	{
		const auto& command_buffer = impl->cmd_buffers[impl->current_frame];

		command_buffer.bindVertexBuffers(0, { impl->vertex_buffer }, { 0 });

		PushConstantData push_constant_data{};
		push_constant_data.model = glm::make_mat4(transform);
		push_constant_data.view = glm::lookAt(glm::vec3(0.0f, 5.0f, 90.0f), glm::vec3(0.0f, 8.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		push_constant_data.projection = glm::perspective(glm::radians(90.0f), 1.7777f, 0.1f, 1000.0f);
		push_constant_data.projection[1][1] *= -1;

		command_buffer.pushConstants(impl->pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantData), &push_constant_data);

		command_buffer.draw(36, 1, 0, 0);
	}
}