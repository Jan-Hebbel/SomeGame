#include "game/game.hpp"

#include "types.hpp"
#include "math.hpp"
#include "game/game_internal.hpp"
#include "platform/platform.hpp"

Perf_Metrics perf_metrics = {};

void game_update(Game_State *game_state, real64 delta_time)
{
	//Input_State input_state = input_queue.next();
	//if (input_state & FORWARD)
	//{
	//	game_state->player.position.y += 0.1f * delta_time * game_state->player.speed;
	//}
	//if (input_state & LEFT)
	//{
	//	game_state->player.position.x -= 0.1f * delta_time * game_state->player.speed;
	//}
	//if (input_state & RIGHT)
	//{
	//	game_state->player.position.x += 0.1f * delta_time * game_state->player.speed;
	//}
	//if (input_state & BACKWARD)
	//{
	//	game_state->player.position.y -= 0.1f * delta_time * game_state->player.speed;
	//}
	static bool walk_right = true;
	if (walk_right) {
		game_state->player.position.x += 5.0f * (float)delta_time;
		if (game_state->player.position.x >= 4.0f) {
			walk_right = false;
		}
	} 
	else {
		game_state->player.position.x -= 5.0f * (float)delta_time;
		if (game_state->player.position.x <= -4.0f) {
			walk_right = true;
			platform_audio_play_file("res/audio/test.wav");
		}
	}
}

void game_render(Game_State *game_state)
{
	Window_Dimensions dimensions = {};
	platform_get_window_dimensions(&dimensions);
	if (dimensions.width == 0 || dimensions.height == 0) return;

	draw_frame(game_state);
}
