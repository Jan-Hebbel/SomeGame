#include "pch.hpp"

#include "game/game.hpp"
#include "game/game_vulkan.hpp"
#include "game/game_vulkan_helper.cpp"

#include <vulkan/vulkan.h>

#include <vector>
#include <cstring>

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

bool32 game_vulkan_init()
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

		if (enable_validation_layers)
		{
			if (check_layer_support(layers, layer_count))
			{
				instance_info.enabledLayerCount = layer_count;
				instance_info.ppEnabledLayerNames = layers;

				instance_info.pNext; // debug callback

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

		if (vkCreateInstance(&instance_info, 0, &vk_struct.instance))
		{
			// TODO: Log (Failed to create vulkan instance! Exiting Game!)
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
