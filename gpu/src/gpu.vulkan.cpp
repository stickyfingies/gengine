#include "gpu.h"

#include "vulkan-headers.hpp"

#include "utils.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#undef min
#undef max

namespace {

// TODO: get rid of this global!!!
auto instance = vk::Instance{};

const auto FRAMES_IN_FLIGHT = 2;
const auto SWAPCHAIN_SIZE = 3;

/**
 * Utility function to convert a list of gpu::VertexAttribtute into Vulkan attribute descriptions
 */
void transcode_vertex_attributes(
	const std::vector<gpu::VertexAttribute>& attributes_in,
	std::vector<vk::VertexInputAttributeDescription>& vk_attributes_out,
	vk::VertexInputBindingDescription& binding_out)
{

	// There's only one vertex buffer, so hard-code the binding to 0.
	const int VERTEX_BUFFER_BINDING = 0;

	const size_t attribute_count = attributes_in.size();
	vk_attributes_out.clear();
	vk_attributes_out.reserve(attribute_count);

	// Convert each attribute to its vulkan counterpart
	size_t vertex_size = 0;
	for (size_t attribute_idx = 0; attribute_idx < attribute_count; attribute_idx++) {
		const auto attribute_in = attributes_in.at(attribute_idx);
		// [float, float, float]
		if (attribute_in == gpu::VertexAttribute::VEC3_FLOAT) {
			const vk::VertexInputAttributeDescription vk_attribute(
				attribute_idx, VERTEX_BUFFER_BINDING, vk::Format::eR32G32B32Sfloat, vertex_size);
			vk_attributes_out.push_back(vk_attribute);
			vertex_size += 3 * sizeof(float);
		}
		// [float, float]
		else if (attribute_in == gpu::VertexAttribute::VEC2_FLOAT) {
			const vk::VertexInputAttributeDescription vk_attribute(
				attribute_idx, VERTEX_BUFFER_BINDING, vk::Format::eR32G32Sfloat, vertex_size);
			vk_attributes_out.push_back(vk_attribute);
			vertex_size += 2 * sizeof(float);
		}
		// unknown
		else {
			std::cerr << "Error while processing vertex attributes: unknown attribute "
					  << static_cast<int>(attribute_in) << std::endl;
		}
	}

	// Generate the Vulkan vertex binding
	binding_out = vk::VertexInputBindingDescription(
		VERTEX_BUFFER_BINDING, vertex_size, vk::VertexInputRate::eVertex);
}

struct PushConstantData {
	glm::mat4 model;
	glm::mat4 view;
	glm::vec3 color;
};

const auto BUFFER_USAGE_TABLE =
	std::array{vk::BufferUsageFlagBits::eVertexBuffer, vk::BufferUsageFlagBits::eIndexBuffer};

} // namespace

namespace gpu {

struct Buffer {
	vk::Buffer buffer;
	vk::DeviceMemory mem;
	size_t size;
};

struct Image {
	std::string name;
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

struct Geometry {
	BufferHandle vbo;
	BufferHandle ebo;
	unsigned long index_count;
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
		cmdbuf.setViewport(0, {viewport});

		const auto scissor = vk::Rect2D({0, 0}, extent);
		cmdbuf.setScissor(0, {scissor});
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
	RenderDeviceVk(std::shared_ptr<GLFWwindow> window) : window{window}
	{
		static const auto debug = false;

		std::cout << "[info]\t Vulkan renderer initializing >:)" << std::endl;

		std::cout << "[info]\t * Debug: " << debug << std::endl;

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

		for (const auto& ext : extension_names) {
			std::cout << "[info]\t * Extension: " << ext << std::endl;
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
		glfwCreateWindowSurface(instance, window.get(), nullptr, &surface);

		// choose physical device
		{
			const auto physical_devices = instance.enumeratePhysicalDevices();

			physical_device = physical_devices.front();

			const auto properties = physical_device.getProperties();

			std::cout << "[info]\t * " << properties.deviceName << std::endl;
			std::cout << "[info]\t\t * Max push constants: "
					  << properties.limits.maxPushConstantsSize << " bytes" << std::endl;
			std::cout << "[info]\t\t * Max memory allocations: "
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

		init_imgui();
	}

	~RenderDeviceVk()
	{
		std::cout << "~ RenderDeviceVk" << std::endl;

		device.waitIdle();

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		destroy_swapchain();

		for (auto i = 0; i < FRAMES_IN_FLIGHT; ++i) {
			device.destroySemaphore(image_available_semaphores[i]);
			device.destroySemaphore(render_finished_semaphores[i]);
			device.destroyFence(swapchain_fences[i]);
		}

		// TODO(seth) - clean up res_buffers pls :)

		device.destroyDescriptorPool(descpool);
		
		device.destroyDescriptorSetLayout(descset_layout);

		device.destroyDescriptorPool(imgui_pool);

		device.destroyRenderPass(backbuffer_pass);

		device.destroyCommandPool(cmd_pool);

		device.destroy();

		instance.destroySurfaceKHR(surface);

		instance.destroy();
	}

	auto init_imgui() -> void
	{
		const vk::DescriptorPoolSize pool_sizes[]{
			{vk::DescriptorType::eSampler, 1000},
			{vk::DescriptorType::eCombinedImageSampler, 1000},
			{vk::DescriptorType::eSampledImage, 1000},
			{vk::DescriptorType::eStorageImage, 1000},
			{vk::DescriptorType::eUniformTexelBuffer, 1000},
			{vk::DescriptorType::eStorageTexelBuffer, 1000},
			{vk::DescriptorType::eUniformBuffer, 1000},
			{vk::DescriptorType::eStorageBuffer, 1000},
			{vk::DescriptorType::eUniformBufferDynamic, 1000},
			{vk::DescriptorType::eStorageBufferDynamic, 1000},
			{vk::DescriptorType::eInputAttachment, 1000}};

		vk::DescriptorPoolCreateInfo pool_info(
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000, pool_sizes);
		imgui_pool = device.createDescriptorPool(pool_info);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForVulkan(window.get(), true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = instance;
		init_info.PhysicalDevice = physical_device;
		init_info.Device = device;
		init_info.QueueFamily = graphics_queue_idx;
		init_info.Queue = graphics_queue;
		init_info.DescriptorPool = imgui_pool;
		init_info.Subpass = 0;
		init_info.MinImageCount = 2;
		init_info.ImageCount = FRAMES_IN_FLIGHT;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.RenderPass = backbuffer_pass;
		ImGui_ImplVulkan_Init(&init_info);
	}

	auto create_buffer(
		BufferUsage usage, std::size_t stride, std::size_t element_count, const void* data)
		-> BufferHandle override
	{
		if (data == nullptr) {
			return {.id = UINT64_MAX};
		}

		// this assumes the user wants a device local buffer

		auto staging = vk::Buffer{};
		auto staging_mem = vk::DeviceMemory{};

		createBufferVk(
			device,
			physical_device,
			element_count * stride,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			staging,
			staging_mem);

		auto buffer = vk::Buffer{};
		auto buffer_mem = vk::DeviceMemory{};

		createBufferVk(
			device,
			physical_device,
			element_count * stride,
			BUFFER_USAGE_TABLE[static_cast<unsigned int>(usage)] |
				vk::BufferUsageFlagBits::eTransferDst,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			buffer,
			buffer_mem);

		auto data_dst = device.mapMemory(staging_mem, 0, element_count * stride);
		memcpy(data_dst, data, element_count * stride);
		device.unmapMemory(staging_mem);

		copy_buffer(staging, buffer, element_count * stride);

		std::cout << "~ GpuBuffer (staging)" << std::endl;
		device.destroyBuffer(staging);
		device.freeMemory(staging_mem);

		uint64_t buffer_handle = res_buffers.size();
		res_buffers.push_back(new Buffer{buffer, buffer_mem, element_count});
		return {.id = buffer_handle};
	}

	auto destroy_buffer(BufferHandle buffer_handle) -> void override
	{
		if (buffer_handle.id == UINT64_MAX) {
			return;
		}
		Buffer* buffer = res_buffers.at(buffer_handle.id);
		std::cout << "~ GpuBuffer" << std::endl;
		device.destroyBuffer(buffer->buffer);
		device.freeMemory(buffer->mem);
		delete buffer;
		res_buffers[buffer_handle.id] = nullptr;
		// TODO(seth) - add buffer_handle.id to a free list
	}

	auto create_image(
		const std::string& name, int width, int height, int channel_count, unsigned char* data_in)
		-> Image* override
	{
		if (image_cache.find(name) != image_cache.end()) {
			return &image_cache.at(name);
		}

		// TODO - determine if we even need a staging buffer!

		auto staging_buffer = vk::Buffer{};
		auto staging_mem = vk::DeviceMemory{};

		const auto image_buffer_size = width * height * channel_count;

		createBufferVk(
			device,
			physical_device,
			image_buffer_size,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			staging_buffer,
			staging_mem);

		auto data = device.mapMemory(staging_mem, 0, image_buffer_size);
		memcpy(data, data_in, image_buffer_size);
		device.unmapMemory(staging_mem);

		auto image = vk::Image{};
		auto image_mem = vk::DeviceMemory{};

		auto mipLevels = 0u;

		create_image_vk(
			name,
			width,
			height,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
				vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			image,
			image_mem,
			mipLevels);
		transition_image_layout(
			image,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal,
			mipLevels);
		copy_buffer_to_image(staging_buffer, image, width, height);

		// This also handles the final image layout transition
		generate_mipmaps(image, width, height, mipLevels);

		std::cout << "[info]\t ~ GpuBuffer" << std::endl;
		device.destroyBuffer(staging_buffer);
		device.freeMemory(staging_mem);

		const auto image_view = create_image_view(
			image, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, mipLevels);

		const auto sampler = create_sampler(mipLevels);

		const auto gpu_image = Image{name, image, image_view, image_mem, sampler};

		image_cache[name] = gpu_image;

		return &image_cache[name];
	}

	auto generate_mipmaps(vk::Image image, uint32_t width, uint32_t height, uint32_t mipLevels)
		-> void
	{
		auto cmdbuf = begin_one_time_cmdbuf();

		int32_t mipWidth = width;
		int32_t mipHeight = height;

		for (auto i = 1; i < mipLevels; i++) {
			const auto subresource =
				vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, i - 1, 1, 0, 1);
			const auto barrier = vk::ImageMemoryBarrier(
				vk::AccessFlagBits::eTransferWrite,
				vk::AccessFlagBits::eTransferRead,
				vk::ImageLayout::eTransferDstOptimal,
				vk::ImageLayout::eTransferSrcOptimal,
				vk::QueueFamilyIgnored,
				vk::QueueFamilyIgnored,
				image,
				subresource);

			cmdbuf.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eTransfer,
				{},
				{},
				{},
				{barrier});

			const auto src_subresource =
				vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i - 1, 0, 1);
			const auto dst_subresource =
				vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i, 0, 1);
			const auto src_offsets =
				std::array{vk::Offset3D{0, 0, 0}, vk::Offset3D{mipWidth, mipHeight, 1}};
			const auto dst_offsets = std::array{
				vk::Offset3D{0, 0, 0},
				vk::Offset3D{
					mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1}};
			const auto blit =
				vk::ImageBlit(src_subresource, src_offsets, dst_subresource, dst_offsets);

			cmdbuf.blitImage(
				image,
				vk::ImageLayout::eTransferSrcOptimal,
				image,
				vk::ImageLayout::eTransferDstOptimal,
				{blit},
				vk::Filter::eLinear);

			const auto subresource2 =
				vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, i - 1, 1, 0, 1);
			const auto barrier2 = vk::ImageMemoryBarrier(
				vk::AccessFlagBits::eTransferRead,
				vk::AccessFlagBits::eShaderRead,
				vk::ImageLayout::eTransferSrcOptimal,
				vk::ImageLayout::eShaderReadOnlyOptimal,
				vk::QueueFamilyIgnored,
				vk::QueueFamilyIgnored,
				image,
				subresource2);

			cmdbuf.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eFragmentShader,
				{},
				{},
				{},
				{barrier2});

			if (mipWidth > 1) {
				mipWidth /= 2;
			}
			if (mipHeight > 1) {
				mipHeight /= 2;
			}
		}

		const auto subresource =
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, mipLevels - 1, 1, 0, 1);
		const auto barrier = vk::ImageMemoryBarrier(
			vk::AccessFlagBits::eTransferWrite,
			vk::AccessFlagBits::eShaderRead,
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal,
			vk::QueueFamilyIgnored,
			vk::QueueFamilyIgnored,
			image,
			subresource);

		cmdbuf.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eFragmentShader,
			{},
			{},
			{},
			{barrier});

		end_one_time_cmdbuf(cmdbuf);
	}

	auto destroy_all_images() -> void override
	{
		for (auto it = image_cache.begin(); it != image_cache.end();) {
			destroy_image(&(it->second));
			image_cache.erase(it++);
		}
	}

	auto destroy_image(Image* image) -> void
	{
		std::cout << "[info]\t ~ GpuImage " << image->name << std::endl;

		device.destroyImageView(image->view);
		device.destroyImage(image->image);
		device.freeMemory(image->mem);
		device.destroySampler(image->sampler);
	}

	auto create_geometry(
		ShaderPipelineHandle pipeline_handle,
		BufferHandle vertex_buffer_handle,
		BufferHandle index_buffer_handle) -> Geometry* override
	{
		Buffer* vertex_buffer = res_buffers.at(vertex_buffer_handle.id);
		Buffer* index_buffer = res_buffers.at(index_buffer_handle.id);
		return new Geometry{vertex_buffer_handle, index_buffer_handle, index_buffer->size};
	}

	auto destroy_geometry(const Geometry* geometry) -> void override
	{
		destroy_buffer(geometry->vbo);
		destroy_buffer(geometry->ebo);
		delete geometry;
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

	auto
	create_descriptors(ShaderPipelineHandle pipeline_handle, Image* albedo, const glm::vec3& color)
		-> Descriptors* override
	{
		ShaderPipeline* pipeline = res_pipelines.at(pipeline_handle.id);
		std::cout << "[info]\t Descriptor " << albedo->name << " rgb(" << color.r << ", " << color.g
				  << ", " << color.b << ")" << std::endl;

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

	auto create_pipeline(
		std::string_view vert_code,
		std::string_view frag_code,
		const std::vector<VertexAttribute>& vertex_attributes) -> ShaderPipelineHandle override
	{

		if (vert_code.empty() || frag_code.empty()) {
			std::cerr << "[error]\t Shader code is empty!" << std::endl;
			return {.id = UINT64_MAX};
		}

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

		const auto push_const_range = vk::PushConstantRange(
			vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
			0,
			sizeof(PushConstantData));

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

		std::vector<vk::VertexInputAttributeDescription> vk_vertex_attributes;
		vk::VertexInputBindingDescription vk_vertex_binding;
		transcode_vertex_attributes(vertex_attributes, vk_vertex_attributes, vk_vertex_binding);

		const auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo(
			{}, 1, &vk_vertex_binding, vk_vertex_attributes.size(), vk_vertex_attributes.data());

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
			{},
			1,
			/* dynamic */ nullptr,
			1,
			/* dynamic */ nullptr);

		const auto rasterizer = vk::PipelineRasterizationStateCreateInfo(
			{},
			false,
			false,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eBack,
			vk::FrontFace::eCounterClockwise,
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

		const auto dynamic_state =
			vk::PipelineDynamicStateCreateInfo({}, dynamic_states.size(), dynamic_states.data());

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

		auto pipeline = device.createGraphicsPipeline(nullptr, pipeline_info);
		if (pipeline.result != vk::Result::eSuccess) {
			std::cerr << "[error]\t Failed to create graphics pipeline!" << std::endl;
			return {.id = UINT64_MAX};
		}

		device.destroyShaderModule(vert_module);
		device.destroyShaderModule(frag_module);

		// note that here we assume that nothing fucked up, and that
		// pipline.value is the actual pipeline object

		const uint64_t pipeline_handle = res_pipelines.size();
		res_pipelines.push_back(new ShaderPipeline{pipeline_layout, pipeline.value, ubo, ubo_mem});
		std::cout << "[info]\t Pipeline " << pipeline_handle << std::endl;
		return {.id = pipeline_handle};
	}

	auto destroy_pipeline(ShaderPipelineHandle pso_handle) -> void override
	{
		if (pso_handle.id == UINT64_MAX) {
			return;
		}
		ShaderPipeline* pso = res_pipelines.at(pso_handle.id);

		device.waitIdle();

		device.destroyPipeline(pso->pipeline);
		device.destroyPipelineLayout(pso->pipeline_layout);
		device.destroyBuffer(pso->ubo);
		device.freeMemory(pso->ubo_mem);

		std::cout << "[info]\t ~ Pipeline " << pso_handle.id << std::endl;

		// TODO - add pso_handle.id to a free list
		delete pso;
		res_pipelines[pso_handle.id] = nullptr;
	}

	auto render(
		const glm::mat4& view,
		ShaderPipelineHandle pso_handle,
		const std::vector<glm::mat4>& transforms,
		const std::vector<Geometry*>& renderables,
		const std::vector<Descriptors*>& descriptors,
		std::function<void()> gui_code) -> void override
	{
		ShaderPipeline* pso = res_pipelines.at(pso_handle.id);
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		gui_code();

		const auto ctx = alloc_context();
		if (!ctx) {
			return;
		}
		ctx->begin();

		ctx->cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pso->pipeline);

		for (auto i = 0; i < transforms.size(); ++i) {

			ctx->cmdbuf.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				pso->pipeline_layout,
				0,
				descriptors[i]->descset,
				{});

			// Push constants
			const auto push_constant_data =
				PushConstantData{transforms[i], view, descriptors[i]->color};
			ctx->cmdbuf.pushConstants(
				pso->pipeline_layout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				0,
				sizeof(PushConstantData),
				&push_constant_data);

			gpu::Buffer* vbo = res_buffers.at(renderables[i]->vbo.id);
			gpu::Buffer* ebo = res_buffers.at(renderables[i]->ebo.id);
			ctx->bind_geometry_buffers(vbo, ebo);
			ctx->draw(renderables[i]->index_count, 1);
		}

		ImGui::Render();
		ImDrawData* draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(draw_data, ctx->cmdbuf);

		ctx->end();

		execute_context(ctx.get());
	}

	auto alloc_context() -> std::unique_ptr<RenderContextVk>
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

			return std::make_unique<RenderContextVk>(
				cmdbuf, backbuffer_pass, backbuffers[image_idx], extent);
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

private:
	auto begin_one_time_cmdbuf() -> vk::CommandBuffer
	{
		const auto alloc_info =
			vk::CommandBufferAllocateInfo(cmd_pool, vk::CommandBufferLevel::ePrimary, 1);

		const auto cmdbuf = device.allocateCommandBuffers(alloc_info).at(0);

		cmdbuf.begin(vk::CommandBufferBeginInfo{});

		return cmdbuf;
	}

	/// Blocks until commands finish
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
		const std::string& debugName,
		unsigned int width,
		unsigned int height,
		vk::Format format,
		vk::ImageTiling tiling,
		vk::ImageUsageFlags usage,
		vk::MemoryPropertyFlags properties,
		vk::Image& image,
		vk::DeviceMemory& mem,
		uint32_t& mipLevels) -> void
	{
		if (mipLevels == 0) {
			mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
		}

		const auto image_info = vk::ImageCreateInfo(
			{},
			vk::ImageType::e2D,
			format,
			{width, height, 1},
			mipLevels,
			1,
			vk::SampleCountFlagBits::e1,
			tiling,
			usage);

		image = device.createImage(image_info);

		std::cout << "[info]\t GpuImage " << debugName << " (" << width << "x" << height
				  << ") mips:" << mipLevels << " " << to_string(format) << " " << to_string(usage)
				  << std::endl;

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

	auto create_image_view(
		vk::Image image, vk::Format format, vk::ImageAspectFlags aspect, uint32_t mipLevels)
		-> vk::ImageView
	{
		const auto component_mapping = vk::ComponentMapping(
			vk::ComponentSwizzle::eR,
			vk::ComponentSwizzle::eG,
			vk::ComponentSwizzle::eB,
			vk::ComponentSwizzle::eA);
		const auto subresource = vk::ImageSubresourceRange(aspect, 0, mipLevels, 0, 1);
		const auto view_info = vk::ImageViewCreateInfo(
			{}, image, vk::ImageViewType::e2D, format, component_mapping, subresource);

		return device.createImageView(view_info);
	}

	auto create_sampler(uint32_t mipLevels) -> vk::Sampler
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
			static_cast<float>(mipLevels));

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
		vk::Image image,
		vk::Format format,
		vk::ImageLayout old_layout,
		vk::ImageLayout new_layout,
		uint32_t mipCount) -> void
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
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mipCount, 0, 1);
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
		glfwGetWindowSize(window.get(), &width, &height);

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

		swapchain = device.createSwapchainKHR(swapchain_info);

		for (const auto& image : device.getSwapchainImagesKHR(swapchain)) {
			swapchain_images.push_back(
				{"Swapchain Image", // TODO - this may need to be unique per swapchain image
				 image,
				 create_image_view(image, surface_fmt, vk::ImageAspectFlagBits::eColor, 1),
				 nullptr,
				 nullptr});
		}

		std::cout << "[info]\t Swapchain (" << extent.width << "x" << extent.height << ") "
				  << to_string(surface_fmt) << std::endl;
	}

	auto destroy_swapchain() -> void
	{
		// TODO - Not really a fan
		std::cout << "[info]\t ~ GpuImage DepthBuffer" << std::endl;
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
		auto mipLevels = 1u;
		create_image_vk(
			"DepthBuffer",
			extent.width,
			extent.height,
			vk::Format::eD24UnormS8Uint,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			depth_buffer,
			depth_mem,
			mipLevels);

		depth_view = create_image_view(
			depth_buffer, vk::Format::eD24UnormS8Uint, vk::ImageAspectFlagBits::eDepth, mipLevels);
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

	std::shared_ptr<GLFWwindow> window;

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

	vk::DescriptorPool imgui_pool;

	std::vector<vk::Semaphore> image_available_semaphores;
	std::vector<vk::Semaphore> render_finished_semaphores;
	std::vector<vk::Fence> swapchain_fences;
	std::vector<vk::Framebuffer> backbuffers;
	std::vector<vk::CommandBuffer> cmd_buffers;

	std::vector<Buffer> buffers;

	std::vector<Image> swapchain_images;

	std::unordered_map<std::string, Image> image_cache;

	unsigned int current_frame = 0;
	unsigned int image_idx = 0;

	uint32_t graphics_queue_idx = 0u;
	uint32_t present_queue_idx = 0u;

	std::vector<Buffer*> res_buffers;
	std::vector<ShaderPipeline*> res_pipelines;
};

auto RenderDevice::create(std::shared_ptr<GLFWwindow> window) -> std::unique_ptr<RenderDevice>
{
	return std::make_unique<RenderDeviceVk>(window);
}

} // namespace gpu

void gpu::configure_glfw() { glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); }