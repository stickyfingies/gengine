#include "renderer.h"

#include "vulkan-headers.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../stb/stb_image.h"

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
		glm::vec2 uv;

		static auto get_binding_desc()->vk::VertexInputBindingDescription
		{
			const vk::VertexInputBindingDescription binding_desc(0, sizeof(Vertex), vk::VertexInputRate::eVertex);

			return binding_desc;
		}

		static auto get_attribute_descs()->std::array<vk::VertexInputAttributeDescription, 3>
		{
			return
			{
				vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
				vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, norm)),
				vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv))
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
		vk::PipelineLayout pipeline_layout;
		vk::Pipeline pipeline;
		vk::DescriptorPool descpool;
		vk::DescriptorSetLayout descset_layout;
		vk::DescriptorSet descset;
		vk::Image albedo;
		vk::ImageView albedo_view;
		vk::DeviceMemory image_mem;
		vk::Sampler sampler;
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
		cmdbuf.begin(vk::CommandBufferBeginInfo());
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
		cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pso->pipeline_layout, 0, pso->descset, {});
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
		push_constant_data.projection = glm::perspective(glm::radians(90.0f), 0.8888f, 0.1f, 10000.0f);

		push_constant_data.projection[1][1] *= -1;
		push_constant_data.model = transform;

		cmdbuf.pushConstants(pso->pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantData), &push_constant_data);
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

	RendererVk(GLFWwindow* window)
	{
		// create instance
		{
			const vk::ApplicationInfo app_info("App Name", VK_MAKE_VERSION(1, 0, 0), "Engine Name", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0);

			const std::array extension_names
			{
				VK_KHR_SURFACE_EXTENSION_NAME,
				#ifdef WIN32
				VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
				#else
				VK_KHR_XCB_SURFACE_EXTENSION_NAME,
				#endif
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME
			};

			const std::array layer_names { "VK_LAYER_KHRONOS_validation" };

			const vk::InstanceCreateInfo instance_info({}, &app_info, layer_names.size(), layer_names.data(), extension_names.size(), extension_names.data());

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
					[](const auto& qfp)
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

			const std::array queue_priorities = { 1.0f };

			const vk::DeviceQueueCreateInfo device_queue_create_info({}, graphics_queue_idx, 1, queue_priorities.data());

			const std::array extension_names = { "VK_KHR_swapchain" };

			const vk::DeviceCreateInfo device_info({}, 1, &device_queue_create_info, 0, nullptr, extension_names.size(), extension_names.data());

			device = physical_device.createDevice(device_info);

			graphics_queue = device.getQueue(graphics_queue_idx, 0);
			present_queue = device.getQueue(present_queue_idx, 0);
		}

		// create command pool / buffers
		{
			const vk::CommandPoolCreateInfo pool_info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphics_queue_idx);

			cmd_pool = device.createCommandPool(pool_info);

			const vk::CommandBufferAllocateInfo alloc_info(cmd_pool, vk::CommandBufferLevel::ePrimary, FRAMES_IN_FLIGHT);
			const auto alloced_buffers = device.allocateCommandBuffers(alloc_info);

			cmd_buffers.insert(cmd_buffers.end(), alloced_buffers.begin(), alloced_buffers.end());
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

			vk::SwapchainCreateInfoKHR swapchain_info({}, surface, surface_caps.minImageCount, surface_fmt, vk::ColorSpaceKHR::eSrgbNonlinear,
				extent, 1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0, nullptr, pre_transform, composite_alpha, present_mode, true, nullptr);

			const std::array queue_family_indices = { graphics_queue_idx, present_queue_idx };

			if (graphics_queue_idx != present_queue_idx)
			{
				swapchain_info.imageSharingMode = vk::SharingMode::eConcurrent;
				swapchain_info.queueFamilyIndexCount = queue_family_indices.size();
				swapchain_info.pQueueFamilyIndices = queue_family_indices.data();
			}

			swapchain = device.createSwapchainKHR(swapchain_info);

			for (const auto& image : device.getSwapchainImagesKHR(swapchain))
			{
				swapchain_images.push_back(gengine::RenderImage{ image, create_image_view(image, surface_fmt, vk::ImageAspectFlagBits::eColor), nullptr });
			}

			// Create semaphores

			for (auto i = 0; i < FRAMES_IN_FLIGHT; ++i)
			{
				image_available_semaphores.push_back(device.createSemaphore({}));
				render_finished_semaphores.push_back(device.createSemaphore({}));

				swapchain_fences.push_back(device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)));
			}
		}

		// create depth buffer
		{
			create_image(extent.width, extent.height, vk::Format::eD24UnormS8Uint, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depth_buffer, depth_mem);

			depth_view = create_image_view(depth_buffer, vk::Format::eD24UnormS8Uint, vk::ImageAspectFlagBits::eDepth);
		}

		// create render pass
		{
			const vk::AttachmentDescription backbuffer_desc({}, surface_fmt, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);

			const vk::AttachmentDescription depth_desc({}, vk::Format::eD24UnormS8Uint, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

			const std::array attachments = { backbuffer_desc, depth_desc };

			const vk::AttachmentReference backbuffer_ref(0, vk::ImageLayout::eColorAttachmentOptimal);

			const vk::AttachmentReference depth_ref(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

			const std::array attachment_refs = { backbuffer_ref };

			const vk::SubpassDescription fwd_subpass({}, vk::PipelineBindPoint::eGraphics, 0, nullptr, attachment_refs.size(), attachment_refs.data(), nullptr, &depth_ref);

			const std::array subpasses = { fwd_subpass };

			const vk::RenderPassCreateInfo render_pass_info({}, attachments.size(), attachments.data(), subpasses.size(), subpasses.data());

			backbuffer_pass = device.createRenderPass(render_pass_info);
		}

		// create framebuffers
		{
			for (const auto& image : swapchain_images)
			{
				const std::array attachments = { image.view, depth_view };

				const vk::FramebufferCreateInfo framebuffer_info({}, backbuffer_pass, attachments.size(), attachments.data(), extent.width, extent.height, 1);

				backbuffers.push_back(device.createFramebuffer(framebuffer_info));
			}
		}
	}

	~RendererVk()
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

		device.destroyCommandPool(cmd_pool);

		device.destroy();

		instance.destroySurfaceKHR(surface);

		instance.destroy();
	}

	auto create_vertex_buffer(const std::vector<float>& vertices, const std::vector<unsigned int>& indices)->gengine::VertexBuffer* override
	{
		auto buffer = new gengine::VertexBuffer{};

		const auto vbo_size = vertices.size() * sizeof(vertices[0]);
		const auto ebo_size = indices.size() * sizeof(indices[0]);

		vk::Buffer vbo_staging;
		vk::Buffer ebo_staging;
		vk::DeviceMemory vbo_staging_mem;
		vk::DeviceMemory ebo_staging_mem;

		create_buffer(vbo_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vbo_staging, vbo_staging_mem);
		create_buffer(ebo_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, ebo_staging, ebo_staging_mem);

		create_buffer(vbo_size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, buffer->vbo, buffer->vbo_mem);
		create_buffer(ebo_size, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, buffer->ebo, buffer->ebo_mem);

		auto data = device.mapMemory(vbo_staging_mem, 0, vbo_size);
		memcpy(data, vertices.data(), vbo_size);
		device.unmapMemory(vbo_staging_mem);

		data = device.mapMemory(ebo_staging_mem, 0, ebo_size);
		memcpy(data, indices.data(), ebo_size);
		device.unmapMemory(ebo_staging_mem);

		copy_buffer(vbo_staging, buffer->vbo, vbo_size);
		copy_buffer(ebo_staging, buffer->ebo, ebo_size);

		device.destroyBuffer(vbo_staging);
		device.destroyBuffer(ebo_staging);
		device.freeMemory(vbo_staging_mem);
		device.freeMemory(ebo_staging_mem);

		return buffer;
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
		const vk::ShaderModuleCreateInfo module_info({}, code.size(), reinterpret_cast<const uint32_t*>(code.data()));

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
		const vk::DescriptorSetLayoutBinding albedo_binding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);

		const std::array bindings = { albedo_binding };

		const vk::DescriptorSetLayoutCreateInfo descset_layout_info({}, bindings.size(), bindings.data());

		const auto descset_layout = device.createDescriptorSetLayout(descset_layout_info);

		const vk::DescriptorPoolSize pool_size(vk::DescriptorType::eCombinedImageSampler, 1);

		const vk::DescriptorPoolCreateInfo descpool_info({}, 1, 1, &pool_size);

		const auto descpool = device.createDescriptorPool(descpool_info);

		const vk::DescriptorSetAllocateInfo descset_info(descpool, 1, &descset_layout);

		const auto descset = device.allocateDescriptorSets(descset_info).at(0);

		//

		int width;
		int height;
		int channel_count;
		const auto image_data = stbi_load("../data/albedo.png", &width, &height, &channel_count, 0);
		const auto image_size = width * height * 4;

		vk::Buffer staging_buffer;
		vk::DeviceMemory staging_mem;

		create_buffer(image_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, staging_buffer, staging_mem);

		auto data = device.mapMemory(staging_mem, 0, image_size);
		memcpy(data, image_data, image_size);
		device.unmapMemory(staging_mem);

		stbi_image_free(image_data);

		vk::Image albedo;
		vk::DeviceMemory image_mem;

		create_image(width, height, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, albedo, image_mem);
		transition_image_layout(albedo, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		copy_buffer_to_image(staging_buffer, albedo, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		transition_image_layout(albedo, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

		device.destroyBuffer(staging_buffer);
		device.freeMemory(staging_mem);

		const auto albedo_view = create_image_view(albedo, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);

		const auto sampler = create_sampler();

		const vk::DescriptorImageInfo desc_image_info(sampler, albedo_view, vk::ImageLayout::eShaderReadOnlyOptimal);

		const vk::WriteDescriptorSet albedo_write(descset, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &desc_image_info);

		device.updateDescriptorSets(albedo_write, {});

		//

		const vk::PushConstantRange push_const_range(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantData));

		const std::array push_const_ranges = { push_const_range };

		const vk::PipelineLayoutCreateInfo pipeline_layout_info({}, 1, &descset_layout, push_const_ranges.size(), push_const_ranges.data());

		const auto pipeline_layout = device.createPipelineLayout(pipeline_layout_info);

		//

		const vk::PipelineShaderStageCreateInfo vert_shader_info({}, vk::ShaderStageFlagBits::eVertex, vert->module, "main");
		const vk::PipelineShaderStageCreateInfo frag_shader_info({}, vk::ShaderStageFlagBits::eFragment, frag->module, "main");

		const std::array shader_stages{ vert_shader_info, frag_shader_info };

		const auto binding_desc = Vertex::get_binding_desc();
		const auto attribute_descs = Vertex::get_attribute_descs();

		const vk::PipelineVertexInputStateCreateInfo vertex_input_info({}, 1, &binding_desc, attribute_descs.size(), attribute_descs.data());

		const vk::PipelineInputAssemblyStateCreateInfo input_assembly({}, vk::PrimitiveTopology::eTriangleList, false);

		const vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);

		const vk::Rect2D scissor(vk::Offset2D(), extent);

		const vk::PipelineViewportStateCreateInfo viewport_state({}, 1, &viewport, 1, &scissor);

		const vk::PipelineRasterizationStateCreateInfo rasterizer({}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);

		const vk::PipelineMultisampleStateCreateInfo multisampling({}, vk::SampleCountFlagBits::e1);

		const vk::PipelineDepthStencilStateCreateInfo depth_stencil({}, true, true, vk::CompareOp::eLess);

		const std::array color_blend_attachments
		{
			vk::PipelineColorBlendAttachmentState(false, vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
		};

		const vk::PipelineColorBlendStateCreateInfo color_blending({}, false, vk::LogicOp::eCopy, color_blend_attachments.size(), color_blend_attachments.data(), { (0.0f, 0.0f, 0.0f, 0.0f) });

		const vk::GraphicsPipelineCreateInfo pipeline_info
		(
			{},
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

		return new gengine::ShaderPipeline{ pipeline_layout, pipeline, descpool, descset_layout, descset, albedo, albedo_view, image_mem, sampler };
	}

	auto destroy_pipeline(gengine::ShaderPipeline* pso)->void override
	{
		device.waitIdle();

		device.destroyPipeline(pso->pipeline);
		device.destroyPipelineLayout(pso->pipeline_layout);
		device.destroySampler(pso->sampler);
		device.destroyImageView(pso->albedo_view);
		device.destroyImage(pso->albedo);
		device.freeMemory(pso->image_mem);
		device.destroyDescriptorPool(pso->descpool);
		device.destroyDescriptorSetLayout(pso->descset_layout);

		delete pso;
	}

	auto get_swapchain_image()->gengine::RenderImage* override
	{
		image_idx = device.acquireNextImageKHR(swapchain, std::numeric_limits<uint64_t>::max(), image_available_semaphores[current_frame], nullptr).value;

		return &swapchain_images[image_idx];
	}

	auto alloc_cmdlist()->gengine::RenderCmdList* override
	{
		device.waitForFences({ swapchain_fences[current_frame] }, true, std::numeric_limits<uint64_t>::max());

		device.resetFences({ swapchain_fences[current_frame] });

		cmd_buffers[current_frame].reset({});

		image_idx = device.acquireNextImageKHR(swapchain, std::numeric_limits<uint64_t>::max(), image_available_semaphores[current_frame], nullptr).value;

		const auto& cmdbuf = cmd_buffers[current_frame];

		return new RenderCmdListVk(cmdbuf, backbuffer_pass, backbuffers[image_idx], extent);
	}

	auto execute_cmdlist(gengine::RenderCmdList* foo)->void override
	{
		auto cmdlist = reinterpret_cast<RenderCmdListVk*>(foo);

		const vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		const auto cmdbuf = cmdlist->get_cmdbuf();
		const vk::SubmitInfo submit_info(1, &image_available_semaphores[current_frame], &wait_dst_stage_mask, 1, &cmdbuf, 1, &render_finished_semaphores[current_frame]);

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

	auto begin_one_time_cmdbuf() -> vk::CommandBuffer
	{
		const vk::CommandBufferAllocateInfo alloc_info(cmd_pool, vk::CommandBufferLevel::ePrimary, 1);

		const auto cmdbuf = device.allocateCommandBuffers(alloc_info).at(0);

		cmdbuf.begin(vk::CommandBufferBeginInfo{});

		return cmdbuf;
	}

	auto end_one_time_cmdbuf(vk::CommandBuffer cmdbuf) -> void
	{
		cmdbuf.end();

		const vk::SubmitInfo submit_info(0, nullptr, nullptr, 1, &cmdbuf);

		graphics_queue.submit(submit_info, nullptr);
		graphics_queue.waitIdle();

		device.freeCommandBuffers(cmd_pool, cmdbuf);
	}

	auto create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& mem) -> void
	{
		const vk::BufferCreateInfo buffer_info({}, size, usage);
		buffer = device.createBuffer(buffer_info);

		const auto mem_reqs = device.getBufferMemoryRequirements(buffer);

		const vk::MemoryAllocateInfo alloc_info(mem_reqs.size, find_memory_type(physical_device, mem_reqs.memoryTypeBits, static_cast<VkMemoryPropertyFlags>(properties)));

		mem = device.allocateMemory(alloc_info);

		device.bindBufferMemory(buffer, mem, 0);
	}

	auto create_image(unsigned int width, unsigned int height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& mem) -> void
	{
		const vk::ImageCreateInfo image_info({}, vk::ImageType::e2D, format, {width, height, 1}, 1, 1, vk::SampleCountFlagBits::e1, tiling, usage);

		image = device.createImage(image_info);

		const auto mem_reqs = device.getImageMemoryRequirements(image);

		const vk::MemoryAllocateInfo alloc_info(mem_reqs.size, find_memory_type(physical_device, mem_reqs.memoryTypeBits, static_cast<VkMemoryPropertyFlags>(properties)));

		mem = device.allocateMemory(alloc_info);

		device.bindImageMemory(image, mem, 0);
	}

	auto create_image_view(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect) -> vk::ImageView
	{
		const vk::ComponentMapping component_mapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
		const vk::ImageSubresourceRange subresource(aspect, 0, 1, 0, 1);
		const vk::ImageViewCreateInfo view_info({}, image, vk::ImageViewType::e2D, format, component_mapping, subresource);

		return device.createImageView(view_info);
	}

	auto create_sampler() -> vk::Sampler
	{
		const vk::SamplerCreateInfo sampler_info({}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0.0f, 16, true, false, vk::CompareOp::eAlways, 0.0f, 0.0f);

		return device.createSampler(sampler_info);
	}

	auto copy_buffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size) -> void
	{
		auto cmdbuf = begin_one_time_cmdbuf();

		const vk::BufferCopy copy_region(0, 0, size);
		cmdbuf.copyBuffer(src, dst, copy_region);

		end_one_time_cmdbuf(cmdbuf);
	}

	auto transition_image_layout(vk::Image image, vk::Format format, vk::ImageLayout old_layout, vk::ImageLayout new_layout) -> void
	{
		vk::PipelineStageFlags src_stage;
		vk::PipelineStageFlags dst_stage;
		vk::AccessFlags src_access_mask;
		vk::AccessFlags dst_access_mask;

		if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal)
		{
			src_access_mask = {};
			dst_access_mask = vk::AccessFlagBits::eTransferWrite;

			src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
			dst_stage = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			src_access_mask = vk::AccessFlagBits::eTransferWrite;
			dst_access_mask = vk::AccessFlagBits::eShaderRead;

			src_stage = vk::PipelineStageFlagBits::eTransfer;
			dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
		}

		auto cmdbuf = begin_one_time_cmdbuf();

		const vk::ImageSubresourceRange subresource(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
		const vk::ImageMemoryBarrier barrier(src_access_mask, dst_access_mask, old_layout, new_layout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, image, subresource);

		cmdbuf.pipelineBarrier(src_stage, dst_stage, vk::DependencyFlags{}, nullptr, nullptr, barrier);

		end_one_time_cmdbuf(cmdbuf);
	}

	auto copy_buffer_to_image(vk::Buffer buffer, vk::Image image, unsigned int width, unsigned int height) -> void
	{
		auto cmdbuf = begin_one_time_cmdbuf();

		const vk::ImageSubresourceLayers subresource(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
		const vk::BufferImageCopy region(0, 0, 0, subresource, vk::Offset3D{0, 0, 0}, vk::Extent3D{width, height, 1});

		cmdbuf.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

		end_one_time_cmdbuf(cmdbuf);
	}

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
	vk::CommandPool cmd_pool;

	unsigned int graphics_queue_idx;
	unsigned int present_queue_idx;

	std::vector<vk::Semaphore> image_available_semaphores;
	std::vector<vk::Semaphore> render_finished_semaphores;
	std::vector<vk::Fence> swapchain_fences;
	std::vector<vk::Framebuffer> backbuffers;
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
