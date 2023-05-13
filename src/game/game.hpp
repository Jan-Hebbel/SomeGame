#ifndef GAME_H
#define GAME_H

/*
    NOTE: Services that the game provides
*/

bool32 game_vulkan_init();
void game_vulkan_cleanup();
void game_wait_idle();

void game_update();
void game_render();

#endif