#ifndef GAME_H
#define GAME_H

#include "types.hpp"

struct Game_State {

};

struct Game_Memory {
	uint64 permanent_storage_size;
	void *permanent_storage;
	uint64 transient_storage_size;
	void *transient_storage;
};

//
// NOTE: Services that the game provides
//

bool32 game_vulkan_init();
void game_vulkan_cleanup();
void game_wait_idle();

void game_update();
void game_render(real64 ms_per_frame, real64 fps, real64 mcpf);

#endif