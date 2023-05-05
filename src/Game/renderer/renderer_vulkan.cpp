#include "AnEngine/renderer/renderer_vulkan.hpp"

#include <vulkan/vulkan.h>

#include "AnEngine/core/assert.hpp"
#include "AnEngine/core/utils.hpp"
#include "AnEngine/core/logger.hpp"

const char *validation_layers[] =
{
	"VK_LAYER_KHRONOS_validation",
};

struct VkContext
{
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkPhysicalDevice physical_device;
	VkSurfaceKHR surface;
};

VkContext vk_context;

VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT m_severity, VkDebugUtilsMessageTypeFlagsEXT m_types, const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data, void *p_user_data)
{
	std::cerr << p_callback_data->pMessage << '\n';
	return VK_FALSE;
}

void create_instance();
void create_debug_messenger();
void create_surface(PlatformWindow *window);
void choose_physical_device();
void destroy_debug_utils_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks *pAllocator);

void renderer_vulkan_init(PlatformWindow *window)
{
	create_instance();
	create_debug_messenger();
	create_surface(window);
	choose_physical_device();
}

void renderer_vulkan_cleanup()
{
	vkDestroySurfaceKHR(vk_context.instance, vk_context.surface, nullptr);

	if (enable_validation_layers)
		destroy_debug_utils_messenger(vk_context.instance, vk_context.debug_messenger, nullptr);

	vkDestroyInstance(vk_context.instance, nullptr);
}

bool check_validation_layer_support();
void populate_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT &messenger_info);

void create_instance()
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
			std::cerr << "Validation layers requested, but not available!" << '\n';
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
#ifdef AE_PLATFORM_WINDOWS
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		/*
#elif AE_PLATFORM_LINUX
		"VK_KHR_wayland_surface",
#elif AE_PLATFORM_MACOS
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

	VkResult result = vkCreateInstance(&instance_info, nullptr, &vk_context.instance);
	if (result != VK_SUCCESS)
	{
		AE_FATAL("Failed to create Vulkan instance! Result: %d", result);
		exit(1);
	}
}

void create_debug_messenger()
{
	if (!enable_validation_layers) return;

	VkDebugUtilsMessengerCreateInfoEXT messenger_info{};
	populate_messenger_create_info(messenger_info);

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_context.instance, "vkCreateDebugUtilsMessengerEXT");

	if (!func)
	{
		std::cerr << "Failed to manually load vkCreateDebugUtilsMessengerEXT!" << '\n';
		exit(1);
	}

	VkResult result = func(vk_context.instance, &messenger_info, nullptr, &vk_context.debug_messenger);

	if (result != VK_SUCCESS)
	{
		std::cerr << "Failed to create debug utils messenger!" << '\n';
		exit(1);
	}
}

void destroy_debug_utils_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks *p_allocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_context.instance, "vkDestroyDebugUtilsMessengerEXT");

	if (!func)
	{
		std::cerr << "Failed to manually load vkDestroyDebugUtilsMessengerEXT!" << '\n';
		exit(1);
	}

	func(vk_context.instance, vk_context.debug_messenger, nullptr);
}

void create_surface(PlatformWindow *window)
{
	// TODO: wayland and macos surfaces
	VkWin32SurfaceCreateInfoKHR surface_info{};
	surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_info.pNext = NULL;
	surface_info.flags = 0;
	surface_info.hinstance = window->hinstance;
	surface_info.hwnd = window->hwnd;

	VkResult result = vkCreateWin32SurfaceKHR(vk_context.instance, &surface_info, nullptr, &vk_context.surface);
	if (result != VK_SUCCESS)
	{
		std::cerr << "Failed to create win32 surface!" << '\n';
		exit(1);
	}
}

// TODO: this function needs to potentially be changed dependent on the features that this application may need
void choose_physical_device()
{
	uint32_t physical_device_count = 0;
	vkEnumeratePhysicalDevices(vk_context.instance, &physical_device_count, NULL);

	if (physical_device_count == 0)
	{
		std::cerr << "Failed to find any Vulkan drivers" << '\n';
		exit(1);
	}

	// TODO: memory arena allocations
	VkPhysicalDevice physical_devices[10];
	AE_ASSERT_MSG(physical_device_count <= 10, "Number of physical devices is higher than 10");
	vkEnumeratePhysicalDevices(vk_context.instance, &physical_device_count, physical_devices);

	for (unsigned int i = 0; i < physical_device_count; ++i)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			vk_context.physical_device = physical_devices[i];
			break;
		}
		else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			vk_context.physical_device = physical_devices[i];
		}
	}

	AE_ASSERT_MSG(vk_context.physical_device != NULL, "No suitable Vulkan driver found!");

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