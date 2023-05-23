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

typedef char File_Asset;

void *platform_get_window_handles();
void platform_get_window_dimensions(Window_Dimensions *dimensions);
void platform_audio_play_file(const char *file_path);
void platform_log(const char *message, ...);
void platform_error_message_window(const char *title, const char *message);
File_Asset *platform_read_file(const char *file_path, uint32 *bytes_read);
void platform_free_file(File_Asset *file);

#endif