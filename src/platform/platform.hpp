#ifndef PLATFORM_HPP
#define PLATFORM_HPP

/*
	NOTE: Functions that the platform layer provides
*/

// get window handles
void *platform_get_window_handles();

// takes in a path to a .wav file and plays it
void platform_audio_play_file(const char *file_path);

#endif