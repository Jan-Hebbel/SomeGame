#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include "types.hpp"

//
// NOTE: Functions that the platform layer provides
//

struct Window_Dimensions {
	uint width;
	uint height;
};

struct File_Asset {
	char *data;
	uint32 size;
};

void *platform_get_window_handles();
void platform_get_window_dimensions(Window_Dimensions *dimensions);
bool platform_audio_play_file(const char *file_path);
void platform_log(const char *message, ...);
void platform_error_message_window(const char *title, const char *message);
uint32 platform_get_file_size(const char *file_path);
uint32 platform_read_file(const char *file_path, File_Asset *file_asset);
void platform_free_file(File_Asset *file_asset);

void platform_logging_init();
void platform_logging_free();

#endif