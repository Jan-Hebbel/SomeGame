#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include "types.hpp"

//
// NOTE: Functions that the platform layer provides
//

void *platform_get_window_handles();
void platform_get_window_dimensions(uint *width, uint *height);
void platform_audio_play_file(const char *file_path);
void platform_log(const char *message, ...);
void platform_error_message_window(const char *title, const char *message);
uint32 platform_get_file_size(const char *file_path);

#endif