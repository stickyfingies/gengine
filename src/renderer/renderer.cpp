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

namespace
{
	const auto FRAMES_IN_FLIGHT = 2;
	const auto SWAPCHAIN_SIZE = 3;

	auto find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties)->uint32_t
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
		glm::vec3 norm;

		static auto get_binding_desc()->vk::VertexInputBindingDescription
		{
			const vk::VertexInputBindingDescription binding_desc(0, sizeof(Vertex), vk::VertexInputRate::eVertex);

			return binding_desc;
		}

		static auto get_attribute_descs()->std::array<vk::VertexInputAttributeDescription, 2>
		{
			return
			{
				vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
				vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, norm))
			};
		}
	};

	struct PushConstantData
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
	};
}

namespace gengine
{
	struct VertexBuffer
	{
		vk::Buffer vbo;
		vk::Buffer ebo;
		vk::DeviceMemory vbo_mem;
		vk::DeviceMemory ebo_mem;
	};

	struct ShaderPipeline
	{
		vk::PipelineLayout layout;
		vk::Pipeline pipeline;
	};

	struct ShaderModule
	{
		vk::ShaderModule module;
	};

	struct RenderImage
	{
		vk::Image image;
		vk::ImageView view;
		vk::DeviceMemory mem;
	};
}

class RenderCmdListVk final : public gengine::RenderCmdList
{
public:

	RenderCmdListVk(vk::CommandBuffer cmdbuf, vk::RenderPass backbuffer_pass, vk::Framebuffer backbuffer, vk::Extent2D extent)
		: cmdbuf{ cmdbuf }
		, backbuffer_pass{ backbuffer_pass }
		, backbuffer{ backbuffer }
		, extent{ extent }
	{
		//
	}

	~RenderCmdListVk()
	{
		//
	}

	auto start_recording()->void override
	{
		cmdbuf.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlags()));
	}

	auto stop_recording()->void override
	{
		cmdbuf.end();
	}

	auto start_frame(gengine::ShaderPipeline* pso)->void override
	{
		const std::array<vk::ClearValue, 2> clear_values
		{
			vk::ClearColorValue(std::array{ 0.2f, 0.2f, 0.2f, 1.0f }),
			vk::ClearDepthStencilValue(1.0f, 0.0f)
		};

		const vk::RenderPassBeginInfo pass_begin_info(backbuffer_pass, backbuffer, vk::Rect2D(vk::Offset2D(), extent), clear_values.size(), clear_values.data());

		cmdbuf.beginRenderPass(pass_begin_info, vk::SubpassContents::eInline);
	}

	auto end_frame()->void override
	{
		cmdbuf.endRenderPass();
	}

	auto bind_pipeline(gengine::ShaderPipeline* pso)->void override
	{
		cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pso->pipeline);
	}

	auto bind_vertex_buffer(gengine::VertexBuffer* vbo)->void override
	{
		cmdbuf.bindVertexBuffers(0, { vbo->vbo }, { 0 });
		cmdbuf.bindIndexBuffer(vbo->ebo, 0, vk::IndexType::eUint32);
	}

	auto push_constants(gengine::ShaderPipeline* pso, const glm::mat4 transform, const float* view)->void override
	{
		PushConstantData push_constant_data{};
		push_constant_data.view = glm::make_mat4(view);
		push_constant_data.projection = glm::perspective(glm::radians(90.0f), 1.7777f, 0.1f, 1000.0f);
		push_constant_data.projection[1][1] *= -1;
		push_constant_data.model = transform;

		cmdbuf.pushConstants(pso->layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantData), &push_constant_data);
	}

	auto draw(int vertex_count, int instance_count)->void override
	{
		cmdbuf.drawIndexed(vertex_count, instance_count, 0, 0, 0);
	}

	//

	auto get_cmdbuf()->vk::CommandBuffer
	{
		return cmdbuf;
	}

private:

	vk::CommandBuffer cmdbuf;
	vk::RenderPass backbuffer_pass;
	vk::Framebuffer backbuffer;
	vk::Extent2D extent;
};

class RendererVk final : public gengine::Renderer
{
public:

	RendererVk::RendererVk(GLFWwindow* window)
	{
		// create instance
		{
			const auto app_info = vk::ApplicationInfo("App Name", VK_MAKE_VERSION(1, 0, 0), "Engine Name", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0);

			const auto extension_names = std::array{
				VK_KHR_SURFACE_EXTENSION_NAME,
				#ifdef WIN32
				VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
				#else
				VK_KHR_XCB_SURFACE_EXTENSION_NAME,
				#endif
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME
			};

			const auto layer_names = std::array{ "VK_LAYER_KHRONOS_validation" };

			const auto instance_info = vk::InstanceCreateInfo(vk::InstanceCreateFlags{}, &app_info, layer_names.size(), layer_names.data(), extension_names.size(), extension_names.data());

			instance = vk::createInstance(instance_info);
		}

		// create surface
		{
			glfwCreateWindowSurface(instance, window, nullptr, &surface);
		}

		// choose physical device
		{
			const auto physical_devices = instance.enumeratePhysicalDevices();

			physical_device = physical_devices.front();
		}

		// create logical device
		{
			const auto queue_family_properties = physical_device.getQueueFamilyProperties();

			graphics_queue_idx = std::distance(
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

			present_queue_idx = physical_device.getSurfaceSupportKHR(graphics_queue_idx, surface) ? graphics_queue_idx : queue_family_properties.size();

			if (present_queue_idx == queue_family_properties.size())
			{
				for (auto i = 0; i < queue_family_properties.size(); ++i)
				{
					if ((queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) && physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface))
					{
						graphics_queue_idx = i;
						present_queue_idx = i;
						break;
					}
				}
				if (present_queue_idx == queue_family_properties.size())
				{
					for (auto i = 0; i < queue_family_properties.size(); ++i)
					{
						if (physical_device.getSurfaceSupportKHR(i, surface))
						{
							present_queue_idx = i;
							break;
						}
					}
				}
			}

			const std::array<float, 1> queue_priorities = { 1.0f };

			const vk::DeviceQueueCreateInfo device_queue_create_info(vk::DeviceQueueCreateFlags(), graphics_queue_idx, 1, queue_priorities.data());

			const std::array<char const*, 1> extension_names = { "VK_KHR_swapchain" };

			const vk::DeviceCreateInfo device_info(vk::DeviceCreateFlags(), 1, &device_queue_create_info, 0, nullptr, extension_names.size(), extension_names.data());

			device = physical_device.createDevice(device_info);

			graphics_queue = device.getQueue(graphics_queue_idx, 0);
			present_queue = device.getQueue(present_queue_idx, 0);
		}

		// create command pool / buffers
		{
			for (auto i = 0; i < FRAMES_IN_FLIGHT; ++i)
			{
				const vk::CommandPoolCreateInfo pool_info(vk::CommandPoolCreateFlags(), graphics_queue_idx);

				cmd_pools.push_back(device.createCommandPool(pool_info));

				const vk::CommandBufferAllocateInfo alloc_info(cmd_pools[i], vk::CommandBufferLevel::ePrimary, 1);

				cmd_buffers.push_back(device.allocateCommandBuffers(alloc_info).at(0));
			}
		}

		// create swapchain
		{
			int width;
			int height;

			glfwGetWindowSize(window, &width, &height);

			const auto formats = physical_device.getSurfaceFormatsKHR(surface);

			surface_fmt = (formats[0].format == vk::Format::eUndefined) ? vk::Format::eB8G8R8A8Unorm : formats[0].format;

			const auto surface_caps = physical_device.getSurfaceCapabilitiesKHR(surface);

			if (surface_caps.currentExtent.width == std::numeric_limits<uint32_t>::max())
			{
				extent.width = std::clamp(static_cast<uint32_t>(width), surface_caps.minImageExtent.width, surface_caps.maxImageExtent.width);
				extent.height = std::clamp(static_cast<uint32_t>(height), surface_caps.minImageExtent.height, surface_caps.maxImageExtent.height);
			}
			else
			{
				extent = surface_caps.currentExtent;
			}

			present_mode = vk::PresentModeKHR::eFifo;

			const auto available_modes = physical_device.getSurfacePresentModesKHR(surface);

			for (const auto& mode : available_modes)
			{
				if (mode == vk::PresentModeKHR::eMailbox)
				{
					present_mode = mode;
				}
				else if (mode == vk::PresentModeKHR::eImmediate)
				{
					present_mode = mode;
				}
			}

			const auto pre_transform = (surface_caps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ? vk::SurfaceTransformFlagBitsKHR::eIdentity : surface_caps.currentTransform;

			const auto composite_alpha =
				(surface_caps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied :
				(surface_caps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied :
				(surface_caps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ? vk::CompositeAlphaFlagBitsKHR::eInherit : vk::CompositeAlphaFlagBitsKHR::eOpaque;

			vk::SwapchainCreateInfoKHR swapchain_info(vk::SwapchainCreateFlagsKHR(), surface, surface_caps.minImageCount, surface_fmt, vk::ColorSpaceKHR::eSrgbNonlinear,
				extent, 1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0, nullptr, pre_transform, composite_alpha, present_mode, true, nullptr);

			const std::array<uint32_t, 2> queue_family_indices = { graphics_queue_idx, present_queue_idx };

			if (graphics_queue_idx != present_queue_idx)
			{
				swapchain_info.imageSharingMode = vk::SharingMode::eConcurrent;
				swapchain_info.queueFamilyIndexCount = queue_family_indices.size();
				swapchain_info.pQueueFamilyIndices = queue_family_indices.data();
			}

			swapchain = device.createSwapchainKHR(swapchain_info);

			const auto images = device.getSwapchainImagesKHR(swapchain);

			const vk::ComponentMapping component_mapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
			const vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

			for (const auto& image : images)
			{
				const vk::ImageViewCreateInfo view_info(vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, surface_fmt, component_mapping, subresource_range);

				swapchain_images.push_back(gengine::RenderImage{ image, device.createImageView(view_info), nullptr });
			}

			// Create semaphores

			for (auto i = 0; i < FRAMES_IN_FLIGHT; ++i)
			{
				image_available_semaphores.push_back(device.createSemaphore(vk::SemaphoreCreateInfo()));
				render_finished_semaphores.push_back(device.createSemaphore(vk::SemaphoreCreateInfo()));

				swapchain_fences.push_back(device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)));
			}
		}

		// create depth buffer
		{
			const auto depth_format = vk::Format::eD24UnormS8Uint;

			const auto format_properties = physical_device.getFormatProperties(depth_format);

			vk::ImageTiling tiling = vk::ImageTiling::eOptimal;

			const vk::ImageCreateInfo image_info(vk::ImageCreateFlags(), vk::ImageType::e2D, depth_format, vk::Extent3D(extent, 1), 1, 1, vk::SampleCountFlagBits::e1, tiling, vk::ImageUsageFlagBits::eDepthStencilAttachment);

			depth_buffer = device.createImage(image_info);

			const vk::MemoryRequirements mem_reqs = device.getImageMemoryRequirements(depth_buffer);

			depth_mem = device.allocateMemory(vk::MemoryAllocateInfo(mem_reqs.size, find_memory_type(physical_device, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));

			device.bindImageMemory(depth_buffer, depth_mem, 0);

			const vk::ComponentMapping component_mapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
			const vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1);

			depth_view = device.createImageView(vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(), depth_buffer, vk::ImageViewType::e2D, depth_format, component_mapping, subresource_range));
		}

		// create render pass
		{
			const vk::AttachmentDescription backbuffer_desc(vk::AttachmentDescriptionFlags(), surface_fmt, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);

			const vk::AttachmentDescription depth_desc(vk::AttachmentDescriptionFlags(), vk::Format::eD24UnormS8Uint, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

			const std::array<vk::AttachmentDescription, 2> attachments = { backbuffer_desc, depth_desc };

			const vk::AttachmentReference backbuffer_ref(0, vk::ImageLayout::eColorAttachmentOptimal);

			const vk::AttachmentReference depth_ref(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

			const std::array<vk::AttachmentReference, 1> attachment_refs = { backbuffer_ref };

			const vk::SubpassDescription fwd_subpass(vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, 0, nullptr, attachment_refs.size(), attachment_refs.data(), nullptr, &depth_ref);

			const std::array<vk::SubpassDescription, 1> subpasses = { fwd_subpass };

			const vk::RenderPassCreateInfo render_pass_info(vk::RenderPassCreateFlags(), attachments.size(), attachments.data(), subpasses.size(), subpasses.data());

			backbuffer_pass = device.createRenderPass(render_pass_info);
		}

		// create framebuffers
		{
			for (const auto& image : swapchain_images)
			{
				const std::array<vk::ImageView, 2> attachments = { image.view, depth_view };

				const vk::FramebufferCreateInfo framebuffer_info(vk::FramebufferCreateFlags(), backbuffer_pass, attachments.size(), attachments.data(), extent.width, extent.height, 1);

				backbuffers.push_back(device.createFramebuffer(framebuffer_info));
			}
		}
	}

	RendererVk::~RendererVk()
	{
		device.waitIdle();

		for (auto i = 0; i < FRAMES_IN_FLIGHT; ++i)
		{
			device.destroySemaphore(image_available_semaphores[i]);
			device.destroySemaphore(render_finished_semaphores[i]);

			device.destroyFence(swapchain_fences[i]);
		}

		for (const auto& backbuffer : backbuffers)
		{
			device.destroyFramebuffer(backbuffer);
		}

		device.destroyImageView(depth_view);
		device.destroyImage(depth_buffer);
		device.freeMemory(depth_mem);

		device.destroyRenderPass(backbuffer_pass);

		for (const auto& image : swapchain_images)
		{
			device.destroyImageView(image.view);
		}

		device.destroySwapchainKHR(swapchain);

		for (const auto& command_pool : cmd_pools)
		{
			device.destroyCommandPool(command_pool);
		}

		device.destroy();

		instance.destroySurfaceKHR(surface);

		instance.destroy();
	}

	auto create_vertex_buffer(const std::vector<float>& vertices, const std::vector<unsigned int>& indices)->gengine::VertexBuffer* override
	{
		const vk::BufferCreateInfo vbo_info(vk::BufferCreateFlags(), vertices.size() * sizeof(vertices[0]), vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive);
		auto vbo = device.createBuffer(vbo_info);

		const vk::BufferCreateInfo ebo_info(vk::BufferCreateFlags(), indices.size() * sizeof(indices[0]), vk::BufferUsageFlagBits::eIndexBuffer, vk::SharingMode::eExclusive);
		auto ebo = device.createBuffer(ebo_info);

		const auto vbo_mem_reqs = device.getBufferMemoryRequirements(vbo);
		const auto ebo_mem_reqs = device.getBufferMemoryRequirements(ebo);

		const vk::MemoryAllocateInfo vbo_alloc_info(vbo_mem_reqs.size, find_memory_type(physical_device, vbo_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		const vk::MemoryAllocateInfo ebo_alloc_info(ebo_mem_reqs.size, find_memory_type(physical_device, vbo_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

		auto vbo_mem = device.allocateMemory(vbo_alloc_info);
		auto ebo_mem = device.allocateMemory(ebo_alloc_info);

		device.bindBufferMemory(vbo, vbo_mem, 0);
		device.bindBufferMemory(ebo, ebo_mem, 0);

		auto data = device.mapMemory(vbo_mem, 0, vbo_info.size);
		memcpy(data, vertices.data(), vbo_info.size);
		device.unmapMemory(vbo_mem);

		data = device.mapMemory(ebo_mem, 0, ebo_info.size);
		memcpy(data, indices.data(), ebo_info.size);
		device.unmapMemory(ebo_mem);

		return new gengine::VertexBuffer{ vbo, ebo, vbo_mem, ebo_mem };
	}

	auto destroy_vertex_buffer(gengine::VertexBuffer* buffer)->void override
	{
		device.destroyBuffer(buffer->vbo);
		device.destroyBuffer(buffer->ebo);
		device.freeMemory(buffer->vbo_mem);
		device.freeMemory(buffer->ebo_mem);

		delete buffer;
	}

	auto create_shader_module(const std::string_view code)->gengine::ShaderModule* override
	{
		const vk::ShaderModuleCreateInfo module_info(vk::ShaderModuleCreateFlags(), code.size(), reinterpret_cast<const uint32_t*>(code.data()));

		const auto module = device.createShaderModule(module_info);

		return new gengine::ShaderModule{ module };
	}

	auto destroy_shader_module(gengine::ShaderModule* module)->void override
	{
		device.destroyShaderModule(module->module);

		delete module;
	}

	auto create_pipeline(gengine::ShaderModule* vert, gengine::ShaderModule* frag)->gengine::ShaderPipeline* override
	{
		const vk::PushConstantRange push_const_range(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantData));

		const std::array push_const_ranges = { push_const_range };

		const vk::PipelineLayoutCreateInfo layout_info(vk::PipelineLayoutCreateFlags(), 0, nullptr, push_const_ranges.size(), push_const_ranges.data());

		const auto pipeline_layout = device.createPipelineLayout(layout_info);

		//

		const vk::PipelineShaderStageCreateInfo vert_shader_info(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, vert->module, "main");
		const vk::PipelineShaderStageCreateInfo frag_shader_info(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, frag->module, "main");

		const std::array shader_stages{ vert_shader_info, frag_shader_info };

		const auto binding_desc = Vertex::get_binding_desc();
		const auto attribute_descs = Vertex::get_attribute_descs();

		const vk::PipelineVertexInputStateCreateInfo vertex_input_info(vk::PipelineVertexInputStateCreateFlags(), 1, &binding_desc, attribute_descs.size(), attribute_descs.data());

		const vk::PipelineInputAssemblyStateCreateInfo input_assembly(vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList, VK_FALSE);

		const vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);

		const vk::Rect2D scissor(vk::Offset2D(), extent);

		const vk::PipelineViewportStateCreateInfo viewport_state(vk::PipelineViewportStateCreateFlags(), 1, &viewport, 1, &scissor);

		const vk::PipelineRasterizationStateCreateInfo rasterizer(vk::PipelineRasterizationStateCreateFlags(), VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f);

		const vk::PipelineMultisampleStateCreateInfo multisampling(vk::PipelineMultisampleStateCreateFlags(), vk::SampleCountFlagBits::e1);

		const vk::PipelineDepthStencilStateCreateInfo depth_stencil(vk::PipelineDepthStencilStateCreateFlags(), VK_TRUE, VK_TRUE, vk::CompareOp::eLess);

		const std::array color_blend_attachments
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
			&depth_stencil,
			&color_blending,
			nullptr,
			pipeline_layout,
			backbuffer_pass
		);

		const auto pipeline = device.createGraphicsPipeline(nullptr, pipeline_info);

		return new gengine::ShaderPipeline{ pipeline_layout, pipeline };
	}

	auto destroy_pipeline(gengine::ShaderPipeline* pso)->void override
	{
		device.destroyPipeline(pso->pipeline);
		device.destroyPipelineLayout(pso->layout);

		delete pso;
	}

	auto get_swapchain_image()->gengine::RenderImage* override
	{
		image_idx = device.acquireNextImageKHR(swapchain, std::numeric_limits<uint64_t>::max(), image_available_semaphores[current_frame], nullptr).value;

		return &swapchain_images[image_idx];
	}

	auto alloc_cmdlist()->gengine::RenderCmdList* override
	{
		device.waitForFences({ swapchain_fences[current_frame] }, VK_TRUE, std::numeric_limits<uint64_t>::max());

		device.resetFences({ swapchain_fences[current_frame] });

		device.resetCommandPool(cmd_pools[current_frame], vk::CommandPoolResetFlags());

		image_idx = device.acquireNextImageKHR(swapchain, std::numeric_limits<uint64_t>::max(), image_available_semaphores[current_frame], nullptr).value;

		const auto& cmdbuf = cmd_buffers[current_frame];

		return new RenderCmdListVk(cmdbuf, backbuffer_pass, backbuffers[image_idx], extent);
	}

	auto execute_cmdlist(gengine::RenderCmdList* foo)->void override
	{
		auto cmdlist = reinterpret_cast<RenderCmdListVk*>(foo);

		const vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		const vk::SubmitInfo submit_info(1, &image_available_semaphores[current_frame], &wait_dst_stage_mask, 1, &cmdlist->get_cmdbuf(), 1, &render_finished_semaphores[current_frame]);

		graphics_queue.submit({ submit_info }, swapchain_fences[current_frame]);

		const vk::PresentInfoKHR present_info(1, &render_finished_semaphores[current_frame], 1, &swapchain, &image_idx);

		present_queue.presentKHR(present_info);

		current_frame = (current_frame + 1) % FRAMES_IN_FLIGHT;
	}

	auto free_cmdlist(gengine::RenderCmdList* cmdlist)->void override
	{
		delete cmdlist;
	}

private:

	vk::Instance instance;

	VkSurfaceKHR surface;

	vk::SwapchainKHR swapchain;

	vk::Format surface_fmt;

	vk::PresentModeKHR present_mode;

	vk::Extent2D extent;

	vk::PhysicalDevice physical_device;

	vk::Device device;

	vk::RenderPass backbuffer_pass;

	vk::Image depth_buffer;
	vk::ImageView depth_view;
	vk::DeviceMemory depth_mem;

	vk::Queue graphics_queue;
	vk::Queue present_queue;

	int graphics_queue_idx;
	int present_queue_idx;

	std::vector<vk::Semaphore> image_available_semaphores;
	std::vector<vk::Semaphore> render_finished_semaphores;
	std::vector<vk::Fence> swapchain_fences;
	std::vector<vk::Framebuffer> backbuffers;
	std::vector<vk::CommandPool> cmd_pools;
	std::vector<vk::CommandBuffer> cmd_buffers;

	std::vector<gengine::RenderImage> swapchain_images;

	unsigned int current_frame = 0;
	unsigned int image_idx = 0;
};

namespace gengine
{
	auto create_renderer(GLFWwindow* window)->Renderer*
	{
		return new RendererVk(window);
	}

	auto destroy_renderer(Renderer* renderer)->void
	{
		delete renderer;
	}
}