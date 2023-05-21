#include "game/game.hpp"

#include "types.hpp"
#include "game/game_internal.hpp"
#include "platform/platform.hpp"

void game_update()
{
	
}

void game_render(real64 ms_per_frame, real64 fps, real64 mcpf)
{
	draw_frame(ms_per_frame, fps, mcpf);
}
