#include "pch.hpp"

#include "game/game_vulkan.hpp"
#include "platform/platform.hpp"

#include <vulkan/vulkan.h>
#include <windows.h>

struct Win32WindowHandles
{
	HINSTANCE hinstance;
	HWND hwnd;
};

// creates a win32 surface; returns 0 on success and 1 on failure
internal_function bool32 vulkan_surface_create(VkInstance instance, VkSurfaceKHR *p_surface)
{
	Win32WindowHandles *p_window_handles = (Win32WindowHandles *)platform_get_window_handles();

	VkWin32SurfaceCreateInfoKHR surface_info{};
	surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_info.hinstance = GetModuleHandleA(0);
	surface_info.hwnd = p_window_handles->hwnd;

	VkResult result = vkCreateWin32SurfaceKHR(instance, &surface_info, 0, p_surface);
	if (result != VK_SUCCESS)
	{
		return 1;
	}
	
	return 0;
}
