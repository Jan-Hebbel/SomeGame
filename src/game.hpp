#ifndef GAME_H
#define GAME_H

#include "types.hpp"
#include "input.hpp"
#include "math.hpp"

enum Game_Mode {
	MODE_MENU   = 0,
	MODE_PLAY   = 1,
	MODE_EDITOR = 2,
};

struct Game_State {
	bool should_close;
	Game_Mode mode; // @Cleanup: should this be here or should this be a global constant or something else?
	struct {
		Vec2 position;
		real32 speed;
	} player; // @Cleanup: player in Game_State?!
};

struct Game_Memory {
	uint64 permanent_storage_size;
	void *permanent_storage;
	uint64 transient_storage_size;
	void *transient_storage;
};

struct Perf_Metrics {
	real64 ms_per_frame;
	real64 fps;
	real64 mcpf;
};

extern Perf_Metrics perf_metrics;

//
// NOTE: Services that the game provides
//

void game_update(Game_State *game_state, real64 delta_time);

#endif