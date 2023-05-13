#include "pch.hpp"

#include "game/game.hpp"

#include "game/game_vulkan_internal.hpp"
#include "platform/platform.hpp"

void game_update()
{
	
}

void game_render()
{
	bool32 result = draw_frame();
	if (result == 2)
	{
		// 2 means that swapchain is either out of date or suboptimal
		recreate_swapchain();
		return;
	}
	else if (result == GAME_FAILURE)
	{
		// TODO: log failure
		return;
	}
}
