#pragma once

struct PlatformWindow;

void  platform_delta_time();
void platform_create_sound_device();
void platform_create_window(const char *title, int width, int height, PlatformWindow *window);
bool platform_window_should_close();
void platform_process_events();