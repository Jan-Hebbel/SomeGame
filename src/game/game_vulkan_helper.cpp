#include "pch.hpp"

#include "game/game_vulkan.hpp"

#include <vulkan/vulkan.h>

#include <vector>

internal bool check_layer_support(const char *layers[], uint size)
{
	uint32 layer_count = 0;
	vkEnumerateInstanceLayerProperties(&layer_count, 0);

	// TODO: memory arena
	std::vector<VkLayerProperties> layer_properties_dynamic_array;
	vkEnumerateInstanceLayerProperties(&layer_count, layer_properties_dynamic_array.data());

	for (const auto &layer_properties : layer_properties_dynamic_array)
	{
		bool layer_found = false;

		for (uint i = 0; i < layer_count; ++i)
		{
			if (std::strcmp(layer_properties.layerName, layers[i]) == 0)
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

	return true;
}