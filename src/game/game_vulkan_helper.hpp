#ifndef GAME_VULKAN_HELPER_H
#define GAME_VULKAN_HELPER_H

#include "pch.hpp"
#include "game/game_vulkan.hpp"

bool check_layer_support(const char *layers[], uint size);
VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data, void *p_user_data);

#endif