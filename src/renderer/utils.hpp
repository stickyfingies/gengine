#pragma once

#include "vulkan-headers.hpp"
#include <iostream>

inline auto findMemoryType(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties)
	-> uint32_t
{
	auto mem_properties = VkPhysicalDeviceMemoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

	for (auto i = 0; i < mem_properties.memoryTypeCount; ++i) {
		if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	std::cout << "Failed to find suitable memory type" << std::endl;
	return 0;
}

inline auto createBufferVk(
	vk::Device device,
	vk::PhysicalDevice physical_device,
	vk::DeviceSize size,
	vk::BufferUsageFlags usage,
	vk::MemoryPropertyFlags properties,
	vk::Buffer &buffer,
	vk::DeviceMemory &mem) -> void
{
	const auto buffer_info = vk::BufferCreateInfo({}, size, usage);

	buffer = device.createBuffer(buffer_info);

	const auto mem_reqs = device.getBufferMemoryRequirements(buffer);

	const auto alloc_info = vk::MemoryAllocateInfo(
		mem_reqs.size,
		findMemoryType(physical_device, mem_reqs.memoryTypeBits, static_cast<VkMemoryPropertyFlags>(properties)));

	mem = device.allocateMemory(alloc_info);

	device.bindBufferMemory(buffer, mem, 0);

	std::cout << "[info]\t created buffer { size: " << size << ", usage: " << static_cast<uint32_t>(usage) << " }"
			  << std::endl;
}