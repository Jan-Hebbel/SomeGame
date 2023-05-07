#include "game/game.hpp"

#if defined PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined PLATFORM_LINUX
// TODO
#elif defined PLATFORM_MACOS
// TODO
#else 
#error Unsupported Operating System!
#endif
#include <vulkan/vulkan.h>

#ifdef _DEBUG
constexpr bool enable_validation_layers = true;
#elif defined NDEBUG
constexpr bool enable_validation_layers = false;
#else 
#error Unsrecognised build mode!
#endif

bool game_vulkan_init()
{
	// load vulkan dll
	return true;
}