#ifndef PLATFORM_HPP
#define PLATFORM_HPP

/*
	NOTE: Functions that the platform layer provides
*/

// get window handles
void *platform_get_window_handles();

// get window width and height
void platform_get_window_dimensions(uint *width, uint *height);

// takes in a path to a .wav file and plays it
void platform_audio_play_file(const char *file_path);

// log to console
void platform_log(const char *message, ...);

void platform_error_message_window(const char *title, const char *message);

#endif