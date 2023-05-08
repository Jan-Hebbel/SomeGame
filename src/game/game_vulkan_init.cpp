#include "pch.hpp"

#include "game/game.hpp"

#if defined PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined PLATFORM_LINUX
// TODO: support linux
#elif defined PLATFORM_MACOS
// TODO: support macos
#else 
#error Unsupported Operating System!
#endif

#include "platform/platform.hpp"

#include <vulkan/vulkan.h>

#include <vector>

#ifdef _DEBUG
constexpr bool enable_validation_layers = true;
#elif defined NDEBUG
constexpr bool enable_validation_layers = false;
#else 
#error Unsrecognised build mode!
#endif

struct GameVulkanStruct
{
	VkInstance instance;
};

global_variable GameVulkanStruct vk_struct;

int game_vulkan_init()
{
	// ---------- create vulkan instance ----------------
	{
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

		// vulkan extensions
		std::vector<const char *> extensions = {
			VK_KHR_SURFACE_EXTENSION_NAME,
	#ifdef VK_USE_PLATFORM_WIN32_KHR
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	#endif
		};

		// TODO: check for validation layer support, if not supported just proceed without
		if (enable_validation_layers)
		{
			instance_info.enabledLayerCount = layer_count;
			instance_info.ppEnabledLayerNames = layers;

			instance_info.pNext; // debug callback

			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		else
		{
			instance_info.enabledLayerCount = 0;
			instance_info.ppEnabledLayerNames = 0;

			instance_info.pNext = 0;
		}

		instance_info.enabledExtensionCount = (uint32)extensions.size();
		instance_info.ppEnabledExtensionNames = extensions.data();

		if (vkCreateInstance(&instance_info, 0, &vk_struct.instance))
		{
			// TODO: Log
			return 1;
		}
	}
	// --------------------------------------------------



	// ---------- create vulkan debug callback ----------
	{

	}
	// --------------------------------------------------
	return 0;
}