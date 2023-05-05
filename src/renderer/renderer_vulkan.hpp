#pragma once

#ifdef PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "platform/platform.hpp"

#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

struct RendererContext
{
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkPhysicalDevice physical_device;
	VkSurfaceKHR surface;
};

void renderer_vulkan_init(PlatformWindow *window, RendererContext *r_context);
void renderer_vulkan_cleanup(RendererContext *r_context);