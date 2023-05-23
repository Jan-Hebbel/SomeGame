#ifndef GAME_H
#define GAME_H

#include "types.hpp"
#include "math.hpp"

struct Game_State {
	struct {
		Vec2 position;
		real32 speed;
	} player;
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

bool32 game_vulkan_init();
void game_vulkan_cleanup();
void game_wait_idle();

void game_update(Game_State *game_state, real64 delta_time);
void game_render(Game_State *game_state);

#endif