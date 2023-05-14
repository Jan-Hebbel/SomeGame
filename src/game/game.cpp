#include "game/game.hpp"

#include "types.hpp"
#include "game/game_vulkan_internal.hpp"
#include "platform/platform.hpp"

void game_update()
{
	
}

void game_render(real64 ms_per_frame, real64 fps, real64 mcpf)
{
	bool32 result = draw_frame(ms_per_frame, fps, mcpf);
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
