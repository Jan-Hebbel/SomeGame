#ifndef GAME_H
#define GAME_H

/*
    NOTE: Services that the platform layer provides to the game
*/


/*
    NOTE: Services that the game provides to the platform layer
*/

bool game_vulkan_init();

void game_update();
void game_render();

#endif