#pragma once

#ifdef WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include <vulkan/vulkan.hpp>