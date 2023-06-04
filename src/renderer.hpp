#ifndef RENDERER_H
#define RENDERER_H

#include "game.hpp"

bool32 renderer_vulkan_init();
void renderer_vulkan_cleanup();
void renderer_vulkan_wait_idle();

void game_render(Game_State *game_state);

#endif
