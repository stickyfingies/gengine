#include "renderer.h"

#include "vulkan-headers.hpp"

#include "utils.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

#undef min
#undef max

namespace {

// TODO: get rid of this global!!!
auto instance = vk::Instance{};

const auto FRAMES_IN_FLIGHT = 2;
const auto SWAPCHAIN_SIZE = 3;

struct Vertex {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 uv;

	static auto get_binding_desc() -> vk::VertexInputBindingDescription
	{
		const auto binding_desc =
			vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);

		return binding_desc;
	}

	static auto get_attribute_descs() -> std::array<vk::VertexInputAttributeDescription, 3>
	{
		const auto pos_attr = vk::VertexInputAttributeDescription(
			0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos));
		const auto norm_attr = vk::VertexInputAttributeDescription(
			1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, norm));
		const auto uv_attr = vk::VertexInputAttributeDescription(
			2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv));

		return {pos_attr, norm_attr, uv_attr};
	}
};

struct PushConstantData {
	glm::mat4 model;
	glm::mat4 view;
	glm::vec3 color;
};

const auto BUFFER_USAGE_TABLE =
	std::array{vk::BufferUsageFlagBits::eVertexBuffer, vk::BufferUsageFlagBits::eIndexBuffer};

} // namespace

namespace gengine {

struct Buffer {
	vk::Buffer buffer;
	vk::DeviceMemory mem;
};

struct Image {
	vk::Image image;
	vk::ImageView view;
	vk::DeviceMemory mem;
	vk::Sampler sampler;
};

struct ShaderPipeline {
	vk::PipelineLayout pipeline_layout;
	vk::Pipeline pipeline;
	vk::Buffer ubo;
	vk::DeviceMemory ubo_mem;
};

struct Descriptors {
	vk::DescriptorSet descset;
	glm::vec3 color;
};

class RenderContextVk final {
public:
	RenderContextVk(
		vk::CommandBuffer cmdbuf,
		vk::RenderPass backbuffer_pass,
		vk::Framebuffer backbuffer,
		vk::Extent2D extent)
		: cmdbuf{cmdbuf}, backbuffer_pass{backbuffer_pass}, backbuffer{backbuffer}, extent{extent}
	{
		//
	}

	~RenderContextVk()
	{
		//
	}

	auto begin() -> void
	{
		cmdbuf.begin(vk::CommandBufferBeginInfo());

		const auto clear_values = std::array<vk::ClearValue, 2>{
			vk::ClearColorValue(std::array{0.2f, 0.2f, 0.2f, 1.0f}),
			vk::ClearDepthStencilValue(1.0f, 0.0f)};

		const auto pass_begin_info = vk::RenderPassBeginInfo(
			backbuffer_pass,
			backbuffer,
			vk::Rect2D(vk::Offset2D(), extent),
			clear_values.size(),
			clear_values.data());

		cmdbuf.beginRenderPass(pass_begin_info, vk::SubpassContents::eInline);

		const auto viewport = vk::Viewport(0.0f, 0.0f, extent.width, extent.height, 0.0f, 1.0f);
		const auto viewports = std::array{viewport};
		cmdbuf.setViewport(0, viewports);

		const auto scissor = vk::Rect2D({0, 0}, extent);
		const auto scissors = std::array{scissor};
		cmdbuf.setScissor(0, scissors);
	}

	auto end() -> void
	{
		cmdbuf.endRenderPass();
		cmdbuf.end();
	}

	auto bind_geometry_buffers(Buffer* vbo, Buffer* ebo) -> void
	{
		cmdbuf.bindVertexBuffers(0, vbo->buffer, {0});
		cmdbuf.bindIndexBuffer(ebo->buffer, 0, vk::IndexType::eUint32);
	}

	auto draw(int vertex_count, int instance_count) -> void
	{
		cmdbuf.drawIndexed(vertex_count, instance_count, 0, 0, 0);
	}

	//

	auto get_cmdbuf() -> vk::CommandBuffer { return cmdbuf; }

	vk::CommandBuffer cmdbuf;

private:
	vk::RenderPass backbuffer_pass;
	vk::Framebuffer backbuffer;
	vk::Extent2D extent;
};

class RenderDeviceVk final : public RenderDevice {
public:
	RenderDeviceVk(GLFWwindow* window) : window{window}
	{
		static const auto debug = true;

		std::cout << "[info]\t (module:renderer) initializing render backend" << std::endl;

		std::cout << "[info]\t\t debug: " << debug << std::endl;

		const auto app_info = vk::ApplicationInfo(
			"App Name",
			VK_MAKE_VERSION(1, 0, 0),
			"Gengine Vk Render Backend",
			VK_MAKE_VERSION(1, 0, 0),
			VK_API_VERSION_1_0);

		const auto extension_names = std::array{
			VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef WIN32
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
			VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

		std::cout << "[info]\t\t driver extensions:" << std::endl;

		for (const auto& ext : extension_names) {
			std::cout << "[info]\t\t\t " << ext << std::endl;
		}

		const auto layer_names = std::array{"VK_LAYER_KHRONOS_validation"};

		const auto instance_info = vk::InstanceCreateInfo(
			{},
			&app_info,
			static_cast<unsigned int>(debug),
			layer_names.data(),
			extension_names.size(),
			extension_names.data());

		instance = vk::createInstance(instance_info);

		// create surface
		glfwCreateWindowSurface(instance, window, nullptr, &surface);

		// choose physical device
		{
			const auto physical_devices = instance.enumeratePhysicalDevices();

			physical_device = physical_devices.front();

			const auto properties = physical_device.getProperties();

			std::cout << "[info]\t selected physical device" << std::endl;
			std::cout << "[info]\t\t name: " << properties.deviceName << std::endl;
			std::cout << "[info]\t\t limits:" << std::endl;
			std::cout << "[info]\t\t\t push constants: " << properties.limits.maxPushConstantsSize
					  << " bytes" << std::endl;
			std::cout << "[info]\t\t\t memory allocations: "
					  << properties.limits.maxMemoryAllocationCount << std::endl;
		}

		// create logical device
		{
			const auto queue_family_properties = physical_device.getQueueFamilyProperties();

			graphics_queue_idx = std::distance(
				queue_family_properties.begin(),
				std::find_if(
					queue_family_properties.begin(),
					queue_family_properties.end(),
					[](const auto& qfp) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; }));

			present_queue_idx = physical_device.getSurfaceSupportKHR(graphics_queue_idx, surface)
									? graphics_queue_idx
									: queue_family_properties.size();

			if (present_queue_idx == queue_family_properties.size()) {
				for (auto i = 0; i < queue_family_properties.size(); ++i) {
					if ((queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
						physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface)) {
						graphics_queue_idx = i;
						present_queue_idx = i;
						break;
					}
				}
				if (present_queue_idx == queue_family_properties.size()) {
					for (auto i = 0; i < queue_family_properties.size(); ++i) {
						if (physical_device.getSurfaceSupportKHR(i, surface)) {
							present_queue_idx = i;
							break;
						}
					}
				}
			}

			const auto queue_priorities = std::array{1.0f};

			const auto device_queue_create_info =
				vk::DeviceQueueCreateInfo({}, graphics_queue_idx, 1, queue_priorities.data());

			const auto extension_names = std::array{"VK_KHR_swapchain"};

			const auto device_info = vk::DeviceCreateInfo(
				{},
				1,
				&device_queue_create_info,
				0,
				nullptr,
				extension_names.size(),
				extension_names.data());

			device = physical_device.createDevice(device_info);

			graphics_queue = device.getQueue(graphics_queue_idx, 0);
			present_queue = device.getQueue(present_queue_idx, 0);
		}

		// create command pool / buffers
		{
			const auto pool_info = vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphics_queue_idx);

			cmd_pool = device.createCommandPool(pool_info);

			const auto alloc_info = vk::CommandBufferAllocateInfo(
				cmd_pool, vk::CommandBufferLevel::ePrimary, FRAMES_IN_FLIGHT);
			const auto alloced_buffers = device.allocateCommandBuffers(alloc_info);

			cmd_buffers.insert(cmd_buffers.end(), alloced_buffers.begin(), alloced_buffers.end());
		}

		// create swapchain
		create_swapchain();

		// create depth buffer
		create_depth_buffer();

		// create render pass
		{
			const auto backbuffer_desc = vk::AttachmentDescription(
				{},
				surface_fmt,
				vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eClear,
				vk::AttachmentStoreOp::eStore,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eDontCare,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::ePresentSrcKHR);
			const auto depth_desc = vk::AttachmentDescription(
				{},
				vk::Format::eD24UnormS8Uint,
				vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eClear,
				vk::AttachmentStoreOp::eDontCare,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eDontCare,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eDepthStencilAttachmentOptimal);

			const auto attachments = std::array{backbuffer_desc, depth_desc};

			const auto backbuffer_ref =
				vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);
			const auto depth_ref =
				vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
			const auto attachment_refs = std::array{backbuffer_ref};

			const auto fwd_subpass = vk::SubpassDescription(
				{},
				vk::PipelineBindPoint::eGraphics,
				0,
				nullptr,
				attachment_refs.size(),
				attachment_refs.data(),
				nullptr,
				&depth_ref);
			const auto subpasses = std::array{fwd_subpass};

			const auto render_pass_info = vk::RenderPassCreateInfo(
				{}, attachments.size(), attachments.data(), subpasses.size(), subpasses.data());

			backbuffer_pass = device.createRenderPass(render_pass_info);
		}

		// create framebuffers
		create_backbuffers();

		// Create semaphores

		for (auto i = 0; i < FRAMES_IN_FLIGHT; ++i) {
			image_available_semaphores.push_back(device.createSemaphore({}));
			render_finished_semaphores.push_back(device.createSemaphore({}));

			swapchain_fences.push_back(
				device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)));
		}

		descset_layout = create_descriptor_set_layout();

		descpool = create_descriptor_pool();
	}

	~RenderDeviceVk()
	{
		device.waitIdle();

		destroy_swapchain();

		for (auto i = 0; i < FRAMES_IN_FLIGHT; ++i) {
			device.destroySemaphore(image_available_semaphores[i]);
			device.destroySemaphore(render_finished_semaphores[i]);
			device.destroyFence(swapchain_fences[i]);
		}

		device.destroyRenderPass(backbuffer_pass);

		device.destroyCommandPool(cmd_pool);

		device.destroy();

		instance.destroySurfaceKHR(surface);

		std::cout << "[info]\t (module:renderer) shutting down render backend" << std::endl;

		instance.destroy();
	}

	auto create_buffer(const BufferInfo& info, const void* data) -> Buffer* override
	{
		// this assumes the user wants a device local buffer

		auto staging = vk::Buffer{};
		auto staging_mem = vk::DeviceMemory{};

		createBufferVk(
			device,
			physical_device,
			info.element_count * info.stride,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			staging,
			staging_mem);

		auto buffer = vk::Buffer{};
		auto buffer_mem = vk::DeviceMemory{};

		createBufferVk(
			device,
			physical_device,
			info.element_count * info.stride,
			BUFFER_USAGE_TABLE[static_cast<unsigned int>(info.usage)] |
				vk::BufferUsageFlagBits::eTransferDst,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			buffer,
			buffer_mem);

		auto data_dst = device.mapMemory(staging_mem, 0, info.element_count * info.stride);
		memcpy(data_dst, data, info.element_count * info.stride);
		device.unmapMemory(staging_mem);

		copy_buffer(staging, buffer, info.element_count * info.stride);

		device.destroyBuffer(staging);
		device.freeMemory(staging_mem);

		return new Buffer{buffer, buffer_mem};
	}

	auto destroy_buffer(Buffer* buffer) -> void override
	{
		device.destroyBuffer(buffer->buffer);
		device.freeMemory(buffer->mem);

		delete buffer;
	}

	auto create_image(const ImageInfo& info, const void* image_data) -> Image* override
	{
		auto staging_buffer = vk::Buffer{};
		auto staging_mem = vk::DeviceMemory{};

		const auto image_size = info.width * info.height * info.channel_count;

		createBufferVk(
			device,
			physical_device,
			image_size,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			staging_buffer,
			staging_mem);

		auto data = device.mapMemory(staging_mem, 0, image_size);
		memcpy(data, image_data, image_size);
		device.unmapMemory(staging_mem);

		auto image = vk::Image{};
		auto image_mem = vk::DeviceMemory{};

		create_image_vk(
			info.width,
			info.height,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			image,
			image_mem);
		transition_image_layout(
			image,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal);
		copy_buffer_to_image(staging_buffer, image, info.width, info.height);
		transition_image_layout(
			image,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal);

		device.destroyBuffer(staging_buffer);
		device.freeMemory(staging_mem);

		const auto image_view =
			create_image_view(image, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);

		const auto sampler = create_sampler();

		return new Image{image, image_view, image_mem, sampler};
	}

	auto destroy_image(Image* image) -> void override
	{
		device.destroyImageView(image->view);
		device.destroyImage(image->image);
		device.freeMemory(image->mem);
		device.destroySampler(image->sampler);

		delete image;
	}

	auto create_renderable(
		const std::vector<float>& vertices,
		const std::vector<float>& vertices_aux,
		const std::vector<uint32_t> indices) -> Renderable
	{
		auto gpu_data = std::vector<float>{};
		for (int i = 0; i < vertices.size() / 3; i++) {
			const auto v = (i * 3);
			gpu_data.push_back(vertices[v + 0]);
			gpu_data.push_back(-vertices[v + 1]);
			gpu_data.push_back(vertices[v + 2]);
			const auto a = (i * 5);
			gpu_data.push_back(vertices_aux[a + 0]);
			gpu_data.push_back(vertices_aux[a + 1]);
			gpu_data.push_back(vertices_aux[a + 2]);
			gpu_data.push_back(vertices_aux[a + 3]);
			gpu_data.push_back(vertices_aux[a + 4]);
		}

		const auto vbo = create_buffer(
			{gengine::BufferInfo::Usage::VERTEX, sizeof(float), gpu_data.size()}, gpu_data.data());
		const auto ebo = create_buffer(
			{gengine::BufferInfo::Usage::INDEX, sizeof(unsigned int), indices.size()},
			indices.data());

		return Renderable{vbo, ebo, indices.size()};
	}

	auto create_descriptor_set_layout() -> vk::DescriptorSetLayout
	{
		// Buffer
		const auto uniform_binding = vk::DescriptorSetLayoutBinding(
			0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);

		// Texture
		const auto albedo_binding = vk::DescriptorSetLayoutBinding(
			1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);

		// Boilerplate
		const auto bindings = std::array{uniform_binding, albedo_binding};
		const auto descset_layout_info =
			vk::DescriptorSetLayoutCreateInfo({}, bindings.size(), bindings.data());
		return device.createDescriptorSetLayout(descset_layout_info);
	}

	auto create_descriptor_pool() -> vk::DescriptorPool
	{
		const auto sampler_size =
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 10);

		const auto uniform_size = vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 10);

		const auto pool_sizes = std::array{sampler_size, uniform_size};

		const auto descpool_info =
			vk::DescriptorPoolCreateInfo({}, 20, pool_sizes.size(), pool_sizes.data());

		return device.createDescriptorPool(descpool_info);
	}

	auto create_descriptors(ShaderPipeline* pipeline, Image* albedo, const glm::vec3& color) -> Descriptors* override
	{
		// Allocate sets

		const auto descset_info = vk::DescriptorSetAllocateInfo(descpool, 1, &descset_layout);

		const auto descset = device.allocateDescriptorSets(descset_info).at(0);

		// update descriptors

		const auto desc_ubo_info = vk::DescriptorBufferInfo(pipeline->ubo, 0, sizeof(glm::mat4));
		const auto ubo_write = vk::WriteDescriptorSet(
			descset, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &desc_ubo_info);

		const auto desc_image_info = vk::DescriptorImageInfo(
			albedo->sampler, albedo->view, vk::ImageLayout::eShaderReadOnlyOptimal);
		const auto albedo_write = vk::WriteDescriptorSet(
			descset, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &desc_image_info);

		const auto descset_writes = std::array{ubo_write, albedo_write};

		device.updateDescriptorSets(descset_writes, {});

		return new Descriptors{descset, color};
	}

	auto create_pipeline(std::string_view vert_code, std::string_view frag_code)
		-> ShaderPipeline* override
	{
		// Create uniform buffer

		auto ubo = vk::Buffer{};
		auto ubo_mem = vk::DeviceMemory{};

		createBufferVk(
			device,
			physical_device,
			sizeof(glm::mat4),
			vk::BufferUsageFlagBits::eUniformBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			ubo,
			ubo_mem);

		auto proj = glm::perspective(glm::radians(90.0f), 0.8888f, 0.1f, 10000.0f);
		proj[1][1] *= -1;

		{
			auto data = device.mapMemory(ubo_mem, 0, sizeof(glm::mat4));
			memcpy(data, &proj, sizeof(proj));
			device.unmapMemory(ubo_mem);
		}

		// push constant info

		const auto push_const_range =
			vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstantData));

		const auto push_const_ranges = std::array{push_const_range};

		const auto pipeline_layout_info = vk::PipelineLayoutCreateInfo(
			{}, 1, &descset_layout, push_const_ranges.size(), push_const_ranges.data());

		const auto pipeline_layout = device.createPipelineLayout(pipeline_layout_info);

		// compile shaders

		const auto vert_module_info = vk::ShaderModuleCreateInfo(
			{}, vert_code.size(), reinterpret_cast<const uint32_t*>(vert_code.data()));

		const auto vert_module = device.createShaderModule(vert_module_info);

		const auto frag_module_info = vk::ShaderModuleCreateInfo(
			{}, frag_code.size(), reinterpret_cast<const uint32_t*>(frag_code.data()));

		const auto frag_module = device.createShaderModule(frag_module_info);

		const auto vert_shader_info = vk::PipelineShaderStageCreateInfo(
			{}, vk::ShaderStageFlagBits::eVertex, vert_module, "main");
		const auto frag_shader_info = vk::PipelineShaderStageCreateInfo(
			{}, vk::ShaderStageFlagBits::eFragment, frag_module, "main");

		const auto shader_stages = std::array{vert_shader_info, frag_shader_info};

		// general graphics pipeline info

		const auto binding_desc = Vertex::get_binding_desc();

		const auto attribute_descs = Vertex::get_attribute_descs();

		const auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo(
			{}, 1, &binding_desc, attribute_descs.size(), attribute_descs.data());

		const auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo(
			{}, vk::PrimitiveTopology::eTriangleList, false);

		const auto viewport = vk::Viewport(
			0.0f,
			0.0f,
			static_cast<float>(extent.width),
			static_cast<float>(extent.height),
			0.0f,
			1.0f);

		const auto scissor = vk::Rect2D(vk::Offset2D(), extent);

		const auto viewport_state = vk::PipelineViewportStateCreateInfo(
			{}, 1, /* dynamic */ nullptr, 1, /* dynamic */ nullptr);

		const auto rasterizer = vk::PipelineRasterizationStateCreateInfo(
			{},
			false,
			false,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eBack,
			vk::FrontFace::eClockwise,
			false,
			0.0f,
			0.0f,
			0.0f,
			1.0f);

		const auto multisampling =
			vk::PipelineMultisampleStateCreateInfo({}, vk::SampleCountFlagBits::e1);

		const auto depth_stencil =
			vk::PipelineDepthStencilStateCreateInfo({}, true, true, vk::CompareOp::eLess);

		const auto color_blend_attachments = std::array{vk::PipelineColorBlendAttachmentState(
			false,
			vk::BlendFactor::eZero,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::BlendFactor::eZero,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)};

		const auto color_blending = vk::PipelineColorBlendStateCreateInfo(
			{},
			false,
			vk::LogicOp::eCopy,
			color_blend_attachments.size(),
			color_blend_attachments.data(),
			{0.0f, 0.0f, 0.0f, 0.0f});

		const auto dynamic_states =
			std::array{vk::DynamicState::eViewport, vk::DynamicState::eScissor};

		vk::PipelineDynamicStateCreateInfo dynamic_state(
			{}, dynamic_states.size(), dynamic_states.data());

		const auto pipeline_info = vk::GraphicsPipelineCreateInfo(
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
			&dynamic_state,
			pipeline_layout,
			backbuffer_pass);

		// finally create the bloody cunt

		const auto pipeline = device.createGraphicsPipeline(nullptr, pipeline_info).value;

		device.destroyShaderModule(vert_module);
		device.destroyShaderModule(frag_module);

		// note that here we assume that nothing fucked up, and that
		// pipline.value is the actual pipeline object

		return new ShaderPipeline{
			pipeline_layout,
			pipeline,
			ubo,
			ubo_mem,
		};
	}

	auto destroy_pipeline(ShaderPipeline* pso) -> void override
	{
		device.waitIdle();

		device.destroyPipeline(pso->pipeline);
		device.destroyPipelineLayout(pso->pipeline_layout);
		device.destroyBuffer(pso->ubo);
		device.freeMemory(pso->ubo_mem);
		device.destroyDescriptorPool(descpool);
		device.destroyDescriptorSetLayout(descset_layout);

		delete pso;
	}

	auto render(
		const glm::mat4& view,
		ShaderPipeline* pso,
		const std::vector<glm::mat4>& transforms,
		const std::vector<Renderable>& renderables,
		const std::vector<Descriptors*>& descriptors) -> void override
	{
		const auto ctx = alloc_context();
		if (!ctx) {
			return;
		}
		ctx->begin();

		ctx->cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pso->pipeline);

		for (auto i = 0; i < transforms.size(); ++i) {

			ctx->cmdbuf.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics, pso->pipeline_layout, 0, descriptors[i]->descset, {});

			// Push constants
			const auto push_constant_data = PushConstantData{transforms[i], view, descriptors[i]->color};
			ctx->cmdbuf.pushConstants(
				pso->pipeline_layout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				0,
				sizeof(PushConstantData),
				&push_constant_data);

			ctx->bind_geometry_buffers(renderables[i].vbo, renderables[i].ebo);
			ctx->draw(renderables[i].index_count, 1);
		}
		ctx->end();

		execute_context(ctx);
		free_context(ctx);
	}

	auto alloc_context() -> RenderContextVk*
	{
		const auto ok = device.waitForFences(
			swapchain_fences[current_frame], true, std::numeric_limits<uint64_t>::max());

		try {
			const auto next_image = device.acquireNextImageKHR(
				swapchain,
				std::numeric_limits<uint64_t>::max(),
				image_available_semaphores[current_frame],
				nullptr);

			if (next_image.result == vk::Result::eErrorOutOfDateKHR) {
				re_create_swapchain();
				return nullptr;
			}

			image_idx = next_image.value;

			device.resetFences({swapchain_fences[current_frame]});

			const auto& cmdbuf = cmd_buffers[current_frame];
			cmdbuf.reset({});

			return new RenderContextVk(cmdbuf, backbuffer_pass, backbuffers[image_idx], extent);
		}
		catch (vk::OutOfDateKHRError) {
			re_create_swapchain();
			return nullptr;
		}
	}

	auto execute_context(RenderContextVk* foo) -> void
	{
		auto cmdlist = reinterpret_cast<RenderContextVk*>(foo);

		const vk::PipelineStageFlags wait_dst_stage_mask =
			vk::PipelineStageFlagBits::eColorAttachmentOutput;

		const auto cmdbuf = cmdlist->get_cmdbuf();
		const auto submit_info = vk::SubmitInfo(
			1,
			&image_available_semaphores[current_frame],
			&wait_dst_stage_mask,
			1,
			&cmdbuf,
			1,
			&render_finished_semaphores[current_frame]);

		graphics_queue.submit({submit_info}, swapchain_fences[current_frame]);

		const auto present_info = vk::PresentInfoKHR(
			1, &render_finished_semaphores[current_frame], 1, &swapchain, &image_idx);

		try {
			const auto result = present_queue.presentKHR(present_info);
			if (result == vk::Result::eSuboptimalKHR) {
				re_create_swapchain();
			}
		}
		catch (vk::OutOfDateKHRError) {
			re_create_swapchain();
		}

		current_frame = (current_frame + 1) % FRAMES_IN_FLIGHT;
	}

	auto free_context(RenderContextVk* ctx) -> void { delete ctx; }

private:
	auto begin_one_time_cmdbuf() -> vk::CommandBuffer
	{
		const auto alloc_info =
			vk::CommandBufferAllocateInfo(cmd_pool, vk::CommandBufferLevel::ePrimary, 1);

		const auto cmdbuf = device.allocateCommandBuffers(alloc_info).at(0);

		cmdbuf.begin(vk::CommandBufferBeginInfo{});

		return cmdbuf;
	}

	auto end_one_time_cmdbuf(vk::CommandBuffer cmdbuf) -> void
	{
		cmdbuf.end();

		const auto submit_info = vk::SubmitInfo(0, nullptr, nullptr, 1, &cmdbuf);

		auto wait_fence = device.createFence({});

		graphics_queue.submit(submit_info, wait_fence);

		const auto ok =
			device.waitForFences(wait_fence, true, std::numeric_limits<uint64_t>::max());

		device.destroyFence(wait_fence);

		device.freeCommandBuffers(cmd_pool, cmdbuf);
	}

	auto create_image_vk(
		unsigned int width,
		unsigned int height,
		vk::Format format,
		vk::ImageTiling tiling,
		vk::ImageUsageFlags usage,
		vk::MemoryPropertyFlags properties,
		vk::Image& image,
		vk::DeviceMemory& mem) -> void
	{
		const auto image_info = vk::ImageCreateInfo(
			{},
			vk::ImageType::e2D,
			format,
			{width, height, 1},
			1,
			1,
			vk::SampleCountFlagBits::e1,
			tiling,
			usage);

		image = device.createImage(image_info);

		const auto mem_reqs = device.getImageMemoryRequirements(image);

		const auto alloc_info = vk::MemoryAllocateInfo(
			mem_reqs.size,
			findMemoryType(
				physical_device,
				mem_reqs.memoryTypeBits,
				static_cast<VkMemoryPropertyFlags>(properties)));

		mem = device.allocateMemory(alloc_info);

		device.bindImageMemory(image, mem, 0);
	}

	auto create_image_view(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect)
		-> vk::ImageView
	{
		const auto component_mapping = vk::ComponentMapping(
			vk::ComponentSwizzle::eR,
			vk::ComponentSwizzle::eG,
			vk::ComponentSwizzle::eB,
			vk::ComponentSwizzle::eA);
		const auto subresource = vk::ImageSubresourceRange(aspect, 0, 1, 0, 1);
		const auto view_info = vk::ImageViewCreateInfo(
			{}, image, vk::ImageViewType::e2D, format, component_mapping, subresource);

		return device.createImageView(view_info);
	}

	auto create_sampler() -> vk::Sampler
	{
		const auto sampler_info = vk::SamplerCreateInfo(
			{},
			vk::Filter::eLinear,
			vk::Filter::eLinear,
			vk::SamplerMipmapMode::eLinear,
			vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat,
			0.0f,
			false,
			16,
			false,
			vk::CompareOp::eAlways,
			0.0f,
			0.0f);

		return device.createSampler(sampler_info);
	}

	auto copy_buffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size) -> void
	{
		const auto cmdbuf = begin_one_time_cmdbuf();

		const auto copy_region = vk::BufferCopy(0, 0, size);

		cmdbuf.copyBuffer(src, dst, copy_region);

		end_one_time_cmdbuf(cmdbuf);
	}

	auto transition_image_layout(
		vk::Image image, vk::Format format, vk::ImageLayout old_layout, vk::ImageLayout new_layout)
		-> void
	{
		auto src_stage = vk::PipelineStageFlags{};
		auto dst_stage = vk::PipelineStageFlags{};
		auto src_access_mask = vk::AccessFlags{};
		auto dst_access_mask = vk::AccessFlags{};

		if (old_layout == vk::ImageLayout::eUndefined &&
			new_layout == vk::ImageLayout::eTransferDstOptimal) {
			src_access_mask = {};
			dst_access_mask = vk::AccessFlagBits::eTransferWrite;

			src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
			dst_stage = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (
			old_layout == vk::ImageLayout::eTransferDstOptimal &&
			new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
			src_access_mask = vk::AccessFlagBits::eTransferWrite;
			dst_access_mask = vk::AccessFlagBits::eShaderRead;

			src_stage = vk::PipelineStageFlagBits::eTransfer;
			dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
		}

		auto cmdbuf = begin_one_time_cmdbuf();

		const auto subresource =
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
		const auto barrier = vk::ImageMemoryBarrier(
			src_access_mask,
			dst_access_mask,
			old_layout,
			new_layout,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			image,
			subresource);

		cmdbuf.pipelineBarrier(
			src_stage, dst_stage, vk::DependencyFlags{}, nullptr, nullptr, barrier);

		end_one_time_cmdbuf(cmdbuf);
	}

	auto copy_buffer_to_image(
		vk::Buffer buffer, vk::Image image, unsigned int width, unsigned int height) -> void
	{
		auto cmdbuf = begin_one_time_cmdbuf();

		const auto subresource =
			vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
		const auto region = vk::BufferImageCopy(
			0, 0, 0, subresource, vk::Offset3D{0, 0, 0}, vk::Extent3D{width, height, 1});

		cmdbuf.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

		end_one_time_cmdbuf(cmdbuf);
	}

	auto create_swapchain() -> void
	{
		auto width = 0;
		auto height = 0;
		glfwGetWindowSize(window, &width, &height);

		const auto formats = physical_device.getSurfaceFormatsKHR(surface);

		surface_fmt = (formats[0].format == vk::Format::eUndefined) ? vk::Format::eB8G8R8A8Unorm
																	: formats[0].format;

		const auto surface_caps = physical_device.getSurfaceCapabilitiesKHR(surface);

		if (surface_caps.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
			extent.width = std::clamp(
				static_cast<uint32_t>(width),
				surface_caps.minImageExtent.width,
				surface_caps.maxImageExtent.width);
			extent.height = std::clamp(
				static_cast<uint32_t>(height),
				surface_caps.minImageExtent.height,
				surface_caps.maxImageExtent.height);
		}
		else {
			extent = surface_caps.currentExtent;
		}

		std::cout << extent.width << " x " << extent.height << std::endl;

		const auto present_mode = [&] {
			auto best_mode = vk::PresentModeKHR::eFifo;

			const auto available_modes = physical_device.getSurfacePresentModesKHR(surface);

			for (const auto& mode : available_modes) {
				if (mode == best_mode) {
					break;
				}
				if (mode == vk::PresentModeKHR::eMailbox) {
					best_mode = mode;
				}
				else if (mode == vk::PresentModeKHR::eImmediate) {
					best_mode = mode;
				}
			}

			return best_mode;
		}();

		const auto pre_transform =
			(surface_caps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
				? vk::SurfaceTransformFlagBitsKHR::eIdentity
				: surface_caps.currentTransform;

		const auto composite_alpha =
			(surface_caps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
				? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
			: (surface_caps.supportedCompositeAlpha &
			   vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
				? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
			: (surface_caps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
				? vk::CompositeAlphaFlagBitsKHR::eInherit
				: vk::CompositeAlphaFlagBitsKHR::eOpaque;

		auto swapchain_info = vk::SwapchainCreateInfoKHR(
			{},
			surface,
			surface_caps.minImageCount,
			surface_fmt,
			vk::ColorSpaceKHR::eSrgbNonlinear,
			extent,
			1,
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::SharingMode::eExclusive,
			0,
			nullptr,
			pre_transform,
			composite_alpha,
			present_mode,
			true,
			nullptr);

		const auto queue_family_indices = std::array{graphics_queue_idx, present_queue_idx};

		if (graphics_queue != present_queue) {
			swapchain_info.imageSharingMode = vk::SharingMode::eConcurrent;
			swapchain_info.queueFamilyIndexCount = queue_family_indices.size();
			swapchain_info.pQueueFamilyIndices = queue_family_indices.data();
		}

		std::cout << "About to create swapchain" << std::endl;

		swapchain = device.createSwapchainKHR(swapchain_info);

		std::cout << "Created swapchain!" << std::endl;

		for (const auto& image : device.getSwapchainImagesKHR(swapchain)) {
			swapchain_images.push_back(
				{image,
				 create_image_view(image, surface_fmt, vk::ImageAspectFlagBits::eColor),
				 nullptr,
				 nullptr});
		}
	}

	auto destroy_swapchain() -> void
	{
		device.destroyImageView(depth_view);
		device.destroyImage(depth_buffer);
		device.freeMemory(depth_mem);

		for (const auto& backbuffer : backbuffers) {
			device.destroyFramebuffer(backbuffer);
		}
		backbuffers.clear();

		for (const auto& image : swapchain_images) {
			device.destroyImageView(image.view);
		}
		swapchain_images.clear();

		device.destroySwapchainKHR(swapchain);
	}

	auto create_depth_buffer() -> void
	{
		create_image_vk(
			extent.width,
			extent.height,
			vk::Format::eD24UnormS8Uint,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			depth_buffer,
			depth_mem);

		depth_view = create_image_view(
			depth_buffer, vk::Format::eD24UnormS8Uint, vk::ImageAspectFlagBits::eDepth);
	}

	auto create_backbuffers() -> void
	{
		for (const auto& image : swapchain_images) {
			const auto attachments = std::array{image.view, depth_view};

			const auto framebuffer_info = vk::FramebufferCreateInfo(
				{},
				backbuffer_pass,
				attachments.size(),
				attachments.data(),
				extent.width,
				extent.height,
				1);

			backbuffers.push_back(device.createFramebuffer(framebuffer_info));
		}
	}

	auto re_create_swapchain() -> void
	{
		device.waitIdle();
		destroy_swapchain();
		create_swapchain();
		create_depth_buffer();
		create_backbuffers();
	}

	GLFWwindow* window;

	VkSurfaceKHR surface;

	vk::SwapchainKHR swapchain;

	vk::Format surface_fmt;

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

	vk::DescriptorPool descpool;
	vk::DescriptorSetLayout descset_layout;

	std::vector<vk::Semaphore> image_available_semaphores;
	std::vector<vk::Semaphore> render_finished_semaphores;
	std::vector<vk::Fence> swapchain_fences;
	std::vector<vk::Framebuffer> backbuffers;
	std::vector<vk::CommandBuffer> cmd_buffers;

	std::vector<Buffer> buffers;

	std::vector<Image> swapchain_images;

	unsigned int current_frame = 0;
	unsigned int image_idx = 0;

	uint32_t graphics_queue_idx = 0u;
	uint32_t present_queue_idx = 0u;
};

auto RenderDevice::create(GLFWwindow* window) -> RenderDevice*
{
	return new RenderDeviceVk(window);
}

auto RenderDevice::destroy(RenderDevice* device) -> void
{
	delete static_cast<RenderDeviceVk*>(device);
}

} // namespace gengine