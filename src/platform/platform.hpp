#ifndef PLATFORM_HPP
#define PLATFORM_HPP

/*
	Functions that the platform layer provides
*/

// takes in a path to a .wav file and plays it
void platform_play_audio_file(const char *file_path);
int platform_load_dll(char *file_path);

#endif