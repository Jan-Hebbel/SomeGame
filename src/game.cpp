#include "game.hpp"

#include "types.hpp"
#include "input.hpp"
#include "math.hpp"
#include "platform.hpp"

Perf_Metrics perf_metrics = {};

void update_play(Game_State *game_state, real64 delta_time) {
	Vec2 plus_minus = {};
	if (get_key_state(W).is_down) {
		plus_minus.y -= 1.0f;
	}
	if (get_key_state(A).is_down) {
		plus_minus.x -= 1.0f;
	}
	if (get_key_state(S).is_down) {
		plus_minus.y += 1.0f;
	}
	if (get_key_state(D).is_down) {
		plus_minus.x += 1.0f;
	}
	game_state->player.position = game_state->player.position + game_state->player.speed * (float)delta_time * normalize(plus_minus);

	Event current_event;
	Event_Reader event_reader = {};
	while ((current_event = event_queue_next(&event_reader)).key_code != UNKNOWN) {
		if (current_event.key_code == ESCAPE && current_event.key_state.is_down && !current_event.key_state.repeated) {
			game_state->mode = MODE_MENU;
		}
	}
}

void update_menu(Game_State *game_state, real64 delta_time) {
	Event current_event;
	Event_Reader event_reader = {};
	while ((current_event = event_queue_next(&event_reader)).key_code != UNKNOWN) {
		if (current_event.key_code == ESCAPE && current_event.key_state.is_down && !current_event.key_state.repeated) {
			game_state->mode = MODE_PLAY;
		}
	}
}

void game_update(Game_State *game_state, real64 delta_time)
{
	switch (game_state->mode) {
		case MODE_PLAY: {
			update_play(game_state, delta_time);
			break;
		}
		case MODE_MENU: {
			update_menu(game_state, delta_time);
			break;
		}
		default: {
			platform_log("Different game mode state not supported!\n");
			break;
		}
	}

	//Event_Reader event_reader = {};

	//Vec2 plus_minus = {};
	//Event current_event;
	//while ((current_event = event_queue_next(&event_reader)).key_code != UNKNOWN) {
	//	if (game_state->mode == MODE_PLAY && current_event.key_status.is_down) {
	//		switch (current_event.key_code) {
	//			case W: {
	//				plus_minus.y -= 1.0f;
	//			} break;

	//			case A: {
	//				plus_minus.x -= 1.0f;
	//			} break;

	//			case S: {
	//				plus_minus.y += 1.0f;
	//			} break;

	//			case D: {
	//				plus_minus.x += 1.0f;
	//			} break;

	//			case ESCAPE: {
	//				if (!current_event.key_status.repeated) {
	//					game_state->mode = MODE_MENU;
	//				}
	//				//platform_log("bruh1\n");
	//			} break;
	//		}
	//	}
	//	else if (game_state->mode == MODE_MENU) {
	//		if (current_event.key_code == ESCAPE && current_event.key_status.is_down && !current_event.key_status.repeated) {
	//			game_state->mode = MODE_PLAY;
	//			//platform_log("bruh2\n");
	//		}
	//	}
	//}
	//game_state->player.position = game_state->player.position + game_state->player.speed * (float)delta_time * normalize(plus_minus);
}
