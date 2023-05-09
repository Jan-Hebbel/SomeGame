#include "pch.hpp"

#include "game/game.hpp"
#include "game/game_vulkan.hpp"
#include "platform/platform.hpp"

// TODO: other operating systems
#ifdef PLATFORM_WINDOWS
// TODO: this cpp file includes windows.h; maybe there is another way of getting things up and running without including windows.h in this file
#include "game/game_vulkan_surface_win32.cpp"
#endif

#include <vulkan/vulkan.h>

#include <vector>
#include <cstring>
#include <optional>

#ifdef _DEBUG
constexpr bool enable_validation_layers = true;
#elif defined NDEBUG
constexpr bool enable_validation_layers = false;
#else 
#error Unsrecognised build mode!
#endif

struct GameVulkanContext
{
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_callback;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical_device;
	VkDevice device;
};

global_variable GameVulkanContext context;

struct QueueFamilyIndices
{
	std::optional<uint32> graphics_family;
	// fill this out as the need for more queue families arises
};

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data, void *p_user_data)
{
	platform_log("%s\n\r", p_callback_data->pMessage);

	return VK_FALSE;
}

bool32 game_vulkan_init()
{
	// vulkan extensions
	std::vector<const char *> extensions = {
		VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
	};

	// debug callback: which messages are filtered and which are not
	VkDebugUtilsMessengerCreateInfoEXT messenger_info{};
	messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messenger_info.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messenger_info.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messenger_info.pfnUserCallback = vulkan_debug_callback;

	QueueFamilyIndices queue_family_indices;

	{
		// create vulkan instance

		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "Game"; // TODO: real name
		app_info.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
		app_info.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo instance_info{};
		instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_info.pApplicationInfo = &app_info;

		// vulkan layers
		const char *layers[] = {
			"VK_LAYER_KHRONOS_validation",
		};
		const uint32 layer_count = sizeof(layers) / sizeof(layers[0]);

		if (enable_validation_layers)
		{
			// check layer support
			bool layers_supported = false;
			uint32 layer_count = 0;
			vkEnumerateInstanceLayerProperties(&layer_count, 0);

			// TODO: memory arena
			std::vector<VkLayerProperties> layer_properties_dynamic_array;
			vkEnumerateInstanceLayerProperties(&layer_count, layer_properties_dynamic_array.data());

			for (const auto &layer_properties : layer_properties_dynamic_array)
			{
				for (uint i = 0; i < layer_count; ++i)
				{
					if (std::strcmp(layer_properties.layerName, layers[i]) == 0)
					{
						bool layers_supported = true;
						break;
					}
				}
			}

			if (layers_supported)
			{
				instance_info.enabledLayerCount = layer_count;
				instance_info.ppEnabledLayerNames = layers;

				instance_info.pNext = &messenger_info;

				extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}
			else
			{
				// TODO: Logging (enable_validation_layers is true but validation layers not supported; continuing without validation layers)
				instance_info.enabledLayerCount = 0;
				instance_info.ppEnabledLayerNames = 0;

				instance_info.pNext = 0;
			}
		}
		else
		{
			instance_info.enabledLayerCount = 0;
			instance_info.ppEnabledLayerNames = 0;

			instance_info.pNext = 0;
		}

		instance_info.enabledExtensionCount = (uint32)extensions.size();
		instance_info.ppEnabledExtensionNames = extensions.data();

		if (vkCreateInstance(&instance_info, 0, &context.instance))
		{
			// TODO: Logging (Failed to create vulkan instance! Exiting Game!)
			return GAME_FAILURE;
		}
		// TODO: Logging (successfully created vulkan instance)
	}



	{
		// create vulkan debug callback
		if (enable_validation_layers)
		{
			// load vkCreateDebugUtilsMessengerEXT
			auto createDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");

			if (createDebugUtilsMessenger)
			{
				// successfully loaded
				VkResult result = createDebugUtilsMessenger(context.instance, &messenger_info, 0, &context.debug_callback);

				if (result == VK_SUCCESS)
				{
					// TODO: Logging (successfully created debug callback)
				}
				else
				{
					// TODO: Logging (Failed to create debug callback; proceeding without it)
				}
			}
			else 
			{
				// TODO: Logging (couldn't load vkCreateDebugUtilsMessengerEXT; proceeding without it)
			}
		}
	}



	{
		// create surface

		bool32 result = vulkan_surface_create(context.instance, &context.surface);
		if (!result)
		{
			// TODO: Logging (successfully created vulkan surface!)
			
		}
		else
		{
			// TODO: Logging (failed to create vulkan surface!)
			return GAME_FAILURE;
		}
	}



	{
		// pick physical device

		uint32 physical_device_count = 0;
		vkEnumeratePhysicalDevices(context.instance, &physical_device_count, 0);

		// TODO: arena allocator
		std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
		vkEnumeratePhysicalDevices(context.instance, &physical_device_count, physical_devices.data());

		for (const auto &physical_device : physical_devices)
		{
			VkPhysicalDeviceProperties physical_device_properties;
			vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

			// find queue families for a physical device
			uint32 queue_family_count;
			vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, 0);

			// TODO: change this when arena allocator is done
			std::vector<VkQueueFamilyProperties> queue_family_properties_array(queue_family_count);
			vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties_array.data());

			int i = 0;
			bool queue_family_indices_is_complete = false;
			for (const auto &queue_family_properties : queue_family_properties_array)
			{
				// TODO: fill this in if need for other queue family arises
				if (queue_family_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					queue_family_indices.graphics_family = i;
				}

				// check if all indices have a value, if they do break the loop early
				if (queue_family_indices.graphics_family.has_value())
				{
					queue_family_indices_is_complete = true;
					break;
				}

				++i;
			}

			// search for other devices if they are not a dedicated gpu
			// TODO: (potentially support multiple graphics cards)
			if (queue_family_indices_is_complete && physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				context.physical_device = physical_device;
				break;
			}
		}

		if (context.physical_device == 0)
		{
			platform_error_message_window("Error!", "Your device has no suitable Vulkan driver!");
			return GAME_FAILURE;
		}

		// TODO: Log the picked physical device
	}



	{
		// create logical device

		// queues to be created with the logical device
		float queue_priority = 1.0f;
		VkDeviceQueueCreateInfo queue_info{};
		queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
		queue_info.queueCount = 1;
		queue_info.pQueuePriorities = &queue_priority;

		// device creation
		VkPhysicalDeviceFeatures device_features{};

		VkDeviceCreateInfo device_info{};
		device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.queueCreateInfoCount = 1;
		device_info.pQueueCreateInfos = &queue_info;
		// TODO: set layer info for older version (since deprecated)?
		device_info.pEnabledFeatures = &device_features;

		VkResult result = vkCreateDevice(context.physical_device, &device_info, 0, &context.device);
		if (result != VK_SUCCESS)
		{
			// TODO: log failure
			
			return 1;
		}

		// TODO: log success
	}

	return GAME_SUCCESS;
}
