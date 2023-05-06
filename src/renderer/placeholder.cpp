// ------------------------- initializing vulkan -------------------------
#if defined PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include <cstring>
#include <cstdlib>

#if defined NDEBUG
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

const char *validation_layers[] =
{
	"VK_LAYER_KHRONOS_validation",
};

bool check_validation_layer_support();
void populate_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT &messenger_info);
void destroy_debug_utils_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks *p_allocator);

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

// initializing vulkan
void game_renderer_init(PlatformWindow *window, RendererContext *context)
{
	// ---- create instance ----
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

	VkResult result = vkCreateInstance(&instance_info, nullptr, &context->instance);
	if (result != VK_SUCCESS)
	{
		AE_FATAL("Failed to create Vulkan instance! Result: %d", result);
		exit(1);
	}
	// -------------------------
	
	// ---- create debug messenger ----
	if (enable_validation_layers)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkCreateDebugUtilsMessengerEXT");
	
		if (!func)
		{
			AE_ERROR("Failed to manually load vkCreateDebugUtilsMessengerEXT!");
			exit(1);
		}
	
		result = func(context->instance, &messenger_info, nullptr, &context->debug_messenger);
	
		if (result != VK_SUCCESS)
		{
			AE_ERROR("Failed to create debug utils messenger!");
			exit(1);
		}
	}
	// --------------------------------
	
	// ---- create surface ----
	// TODO: wayland and macos surfaces
	VkWin32SurfaceCreateInfoKHR surface_info{};
	surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_info.pNext = NULL;
	surface_info.flags = 0;
	surface_info.hinstance = window->hinstance;
	surface_info.hwnd = window->hwnd;

	result = vkCreateWin32SurfaceKHR(context->instance, &surface_info, nullptr, &context->surface);
	if (result != VK_SUCCESS)
	{
		AE_ERROR("Failed to create win32 surface!");
		exit(1);
	}
	// ------------------------
	
	// ---- choose physical device ----
	// needs to potentially be changed dependent on the features that this application may need
	uint32_t physical_device_count = 0;
	vkEnumeratePhysicalDevices(context->instance, &physical_device_count, NULL);

	if (physical_device_count == 0)
	{
		AE_ERROR("Failed to find any Vulkan drivers");
		exit(1);
	}

	// TODO: memory arena allocations
	VkPhysicalDevice physical_devices[10];
	AE_ASSERT_MSG(physical_device_count <= 10, "Number of physical devices is higher than 10");
	vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_devices);

	for (unsigned int i = 0; i < physical_device_count; ++i)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			context->physical_device = physical_devices[i];
			break;
		}
		else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			context->physical_device = physical_devices[i];
		}
	}

	AE_ASSERT_MSG(context->physical_device != NULL, "No suitable Vulkan driver found!");

	// can/does physical device 
	//     - present to a surface?
	//     - support swap chain?
	//     - have graphics queue family?
	// --------------------------------
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

void game_renderer_cleanup(RendererContext *context)
{
	vkDestroySurfaceKHR(context->instance, context->surface, nullptr);

	if (enable_validation_layers)
		destroy_debug_utils_messenger(context->instance, context->debug_messenger, nullptr);

	vkDestroyInstance(context->instance, nullptr);
}

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
// -----------------------------------------------------------------------