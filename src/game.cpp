#include "game.hpp"

#include "types.hpp"
#include "input.hpp"
#include "math.hpp"
#include "vulkan/vulkan_draw.hpp"
#include "platform/platform.hpp"

Perf_Metrics perf_metrics = {};

void game_update(Game_State *game_state, real64 delta_time)
{
	Event_Reader event_reader = {};

	Vec2 plus_minus = {};
	Event current_event;
	while ((current_event = event_queue_next(&event_reader)).key_code != UNKNOWN) {
		switch (current_event.key_code) {
			case W: {
				plus_minus.y -= 1.0f;
			} break;

			case A: {
				plus_minus.x -= 1.0f;
			} break;

			case S: {
				plus_minus.y += 1.0f;
			} break;

			case D: {
				plus_minus.x += 1.0f;
			} break;

			case ESCAPE: {
				game_state->should_close = true;
			} break;
		}
	}
	game_state->player.position = game_state->player.position + game_state->player.speed * (float)delta_time * normalize(plus_minus);

	event_queue_clear();
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
}
