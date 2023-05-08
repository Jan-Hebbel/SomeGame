// include this file for importing the vulkan header after defining the appropriate macro for different operating systems

#ifndef GAME_VULKAN_PCH_H
#define GAME_VULKAN_PCH_H

#if defined PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined PLATFORM_LINUX
// TODO: support linux
#elif defined PLATFORM_MACOS
// TODO: support macos
#else 
#error Unsupported Operating System!
#endif
#include <vulkan/vulkan.h>

#endif