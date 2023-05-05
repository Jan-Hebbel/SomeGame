#include "renderer/renderer_vulkan.hpp"

#include "core/assert.hpp"
#include "core/utils.hpp"
#include "core/logger.hpp"

#include <cstring>
#include <cstdlib>

const char *validation_layers[] =
{
	"VK_LAYER_KHRONOS_validation",
};

VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT m_severity, VkDebugUtilsMessageTypeFlagsEXT m_types, const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data, void *p_user_data)
{
	switch (m_severity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		{
			AE_TRACE("%s", p_callback_data->pMessage);
		} break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		{
			AE_INFO("%s", p_callback_data->pMessage);
		} break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		{
			AE_WARN("%s", p_callback_data->pMessage);
		} break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		{
			AE_ERROR("%s", p_callback_data->pMessage);
		} break;
	}
	return VK_FALSE;
}

void create_instance(RendererContext *r_context);
void create_debug_messenger(RendererContext *r_context);
void create_surface(RendererContext *r_context, PlatformWindow *window);
void choose_physical_device(RendererContext *r_context);
void destroy_debug_utils_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks *pAllocator);

void renderer_vulkan_init(PlatformWindow *window, RendererContext *r_context)
{
	create_instance(r_context);
	create_debug_messenger(r_context);
	create_surface(r_context, window);
	choose_physical_device(r_context);
}

void renderer_vulkan_cleanup(RendererContext *r_context)
{
	vkDestroySurfaceKHR(r_context->instance, r_context->surface, nullptr);

	if (enable_validation_layers)
		destroy_debug_utils_messenger(r_context->instance, r_context->debug_messenger, nullptr);

	vkDestroyInstance(r_context->instance, nullptr);
}

bool check_validation_layer_support();
void populate_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT &messenger_info);

void create_instance(RendererContext *r_context)
{
	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	app_info.pApplicationName = "Something Crazy 3D";
	app_info.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
	app_info.pEngineName = "AnEngine";
	app_info.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
	app_info.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo instance_info{};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.flags = 0;
	instance_info.pApplicationInfo = &app_info;

	VkDebugUtilsMessengerCreateInfoEXT messenger_info{};
	if (enable_validation_layers)
	{
		if (!check_validation_layer_support())
		{
			AE_ERROR("Validation layers requested, but not available!");
			exit(1);
		}

		instance_info.enabledLayerCount = static_cast<uint32_t>(SIZE(validation_layers));
		instance_info.ppEnabledLayerNames = validation_layers;

		populate_messenger_create_info(messenger_info);
		instance_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&messenger_info;
	}
	else
	{
		instance_info.enabledLayerCount = 0;
		instance_info.ppEnabledLayerNames = NULL;

		instance_info.pNext = NULL;
	}

	const char *extensions[3] =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef PLATFORM_WINDOWS
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		/*
#elif PLATFORM_LINUX
		"VK_KHR_wayland_surface",
#elif PLATFORM_MACOS
		"VK_MVK_macos_surface",
		*/
#endif
	};
	if (enable_validation_layers)
	{
		extensions[2] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
		instance_info.enabledExtensionCount = static_cast<uint32_t>(SIZE(extensions));
	}
	else
	{
		instance_info.enabledExtensionCount = 2;
	}
	instance_info.ppEnabledExtensionNames = extensions;

	VkResult result = vkCreateInstance(&instance_info, nullptr, &r_context->instance);
	if (result != VK_SUCCESS)
	{
		AE_FATAL("Failed to create Vulkan instance! Result: %d", result);
		exit(1);
	}
}

void create_debug_messenger(RendererContext *r_context)
{
	if (!enable_validation_layers) return;

	VkDebugUtilsMessengerCreateInfoEXT messenger_info{};
	populate_messenger_create_info(messenger_info);

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(r_context->instance, "vkCreateDebugUtilsMessengerEXT");

	if (!func)
	{
		AE_ERROR("Failed to manually load vkCreateDebugUtilsMessengerEXT!");
		exit(1);
	}

	VkResult result = func(r_context->instance, &messenger_info, nullptr, &r_context->debug_messenger);

	if (result != VK_SUCCESS)
	{
		AE_ERROR("Failed to create debug utils messenger!");
		exit(1);
	}
}

void destroy_debug_utils_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks *p_allocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (!func)
	{
		AE_ERROR("Failed to manually load vkDestroyDebugUtilsMessengerEXT!");
		exit(1);
	}

	func(instance, debug_messenger, nullptr);
}

void create_surface(RendererContext *r_context, PlatformWindow *window)
{
	// TODO: wayland and macos surfaces
	VkWin32SurfaceCreateInfoKHR surface_info{};
	surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_info.pNext = NULL;
	surface_info.flags = 0;
	surface_info.hinstance = window->hinstance;
	surface_info.hwnd = window->hwnd;

	VkResult result = vkCreateWin32SurfaceKHR(r_context->instance, &surface_info, nullptr, &r_context->surface);
	if (result != VK_SUCCESS)
	{
		AE_ERROR("Failed to create win32 surface!");
		exit(1);
	}
}

// TODO: this function needs to potentially be changed dependent on the features that this application may need
void choose_physical_device(RendererContext *r_context)
{
	uint32_t physical_device_count = 0;
	vkEnumeratePhysicalDevices(r_context->instance, &physical_device_count, NULL);

	if (physical_device_count == 0)
	{
		AE_ERROR("Failed to find any Vulkan drivers");
		exit(1);
	}

	// TODO: memory arena allocations
	VkPhysicalDevice physical_devices[10];
	AE_ASSERT_MSG(physical_device_count <= 10, "Number of physical devices is higher than 10");
	vkEnumeratePhysicalDevices(r_context->instance, &physical_device_count, physical_devices);

	for (unsigned int i = 0; i < physical_device_count; ++i)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			r_context->physical_device = physical_devices[i];
			break;
		}
		else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			r_context->physical_device = physical_devices[i];
		}
	}

	AE_ASSERT_MSG(r_context->physical_device != NULL, "No suitable Vulkan driver found!");

	// can/does physical device 
	//     - present to a surface?
	//     - support swap chain?
	//     - have graphics queue family?
}

// --------------------- Vulkan initialization helper functions ---------------------

bool check_validation_layer_support()
{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, NULL);

	// TODO: memory arena
	AE_ASSERT_MSG(layer_count <= 50, "Layer count exceededs the heap memory allocated for it!");
	VkLayerProperties *properties = new VkLayerProperties[50];
	vkEnumerateInstanceLayerProperties(&layer_count, properties);

	for (const char *layer : validation_layers)
	{
		bool layer_found = false;

		for (uint8_t i = 0; i < 50; ++i)
		{
			if (std::strcmp(layer, properties[i].layerName) == 0)
			{
				layer_found = true;
				break;
			}
		}

		if (!layer_found)
		{
			return false;
		}
	}
	delete[] properties;

	return true;
}

void populate_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT &messenger_info)
{
	messenger_info = {};
	messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messenger_info.pNext = NULL;
	messenger_info.flags = 0;
	messenger_info.messageSeverity =
		/* VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | */
		/* VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | */
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messenger_info.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messenger_info.pfnUserCallback = debug_callback;
	messenger_info.pUserData = NULL;
}