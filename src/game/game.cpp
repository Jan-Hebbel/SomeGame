#include "game/game.hpp"

#include "types.hpp"
#include "input.hpp"
#include "math.hpp"
#include "game/game_internal.hpp"
#include "platform/platform.hpp"

Perf_Metrics perf_metrics = {};

void game_update(Game_State *game_state, real64 delta_time)
{
	Event_Reader event_reader = {};

	Event current_event;
	while ((current_event = event_queue_next(&event_reader)).key_code != UNKNOWN) {
		switch (current_event.key_code) {
			case W: {
				if (current_event.key_is_down)
					game_state->player.position.y -= game_state->player.speed * (float)delta_time;
			} break;

			case A: {
				if (current_event.key_is_down)
					game_state->player.position.x -= game_state->player.speed * (float)delta_time;
			} break;

			case S: {
				if (current_event.key_is_down)
					game_state->player.position.y += game_state->player.speed * (float)delta_time;
			} break;

			case D: {
				if (current_event.key_is_down)
					game_state->player.position.x += game_state->player.speed * (float)delta_time;
			} break;
		}
	}

	event_queue_clear();

	//static bool walk_right = true;
	//if (walk_right) {
	//	game_state->player.position.x += 1.0f * game_state->player.speed * (float)delta_time;
	//	if (game_state->player.position.x >= 4.0f) {
	//		walk_right = false;
	//	}
	//} 
	//else {
	//	game_state->player.position.x -= 1.0f * game_state->player.speed * (float)delta_time;
	//	if (game_state->player.position.x <= -4.0f) {
	//		walk_right = true;
	//		platform_audio_play_file("res/audio/test.wav");
	//	}
	//}
}

void game_render(Game_State *game_state)
{
	Window_Dimensions dimensions = {};
	platform_get_window_dimensions(&dimensions);
	if (dimensions.width == 0 || dimensions.height == 0) return;

	switch (game_state->mode)
	{
		case GAME_PLAY:
		{
			draw_frame(game_state);
		} break;

		default:
		{
			platform_log("Different Game Modes not yet supported!");
		} break;
	}

	draw_frame(game_state);
}
