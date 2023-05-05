#include "platform/platform.hpp"

#include "core/logger.hpp"
#include "renderer/renderer_vulkan.hpp"

void platform_game_create(PlatformWindow *window)
{
	logging_initialize();

	platform_create_window("SomeGame", 1280, 720, window);
	platform_create_sound_device();	
}

void platform_game_loop(PlatformWindow *window, RendererContext *r_context)
{
	while (!platform_window_should_close())
	{
		platform_process_events();
		
		//update();
		//renderer_render();
	
		platform_delta_time();
	}
}

void platform_game_cleanup(PlatformWindow *window, RendererContext *r_context)
{
	// TODO: potentially destroy window?
	//platform_destroy_window(window);
	renderer_vulkan_cleanup(r_context);
}