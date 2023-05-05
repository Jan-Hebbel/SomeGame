#include "core/application.hpp"

#include "renderer/renderer_vulkan.hpp"
#include "platform/platform_win32.cpp"

#include "core/logger.hpp"
#include "core/assert.hpp"

#include <stdint.h>

int64_t perf_count_frequency;

namespace AnEngine
{
	void application_create()
	{
		// logging init
		logging_initialize();

		LARGE_INTEGER perf_count_frequency_result;
		QueryPerformanceFrequency(&perf_count_frequency_result);
		perf_count_frequency = perf_count_frequency_result.QuadPart;

		PlatformWindow window;
		platform_create_window("Sandbox", 1280, 720, &window);

		renderer_vulkan_init(&window);
	}

	void application_run()
	{
		LARGE_INTEGER last_counter;
		QueryPerformanceCounter(&last_counter);

		int64_t last_cycle_count = __rdtsc();

		// game loop
		while (!platform_window_should_close())
		{
			// process events => (input and window messages)
			platform_process_events();

			// update()
			// render()

			// calculate frame time
			//float frame_time = delta_time();
			int64_t end_cycle_count = __rdtsc();

			LARGE_INTEGER end_counter;
			QueryPerformanceCounter(&end_counter);

			// TODO: display values on screen
			int64_t cycles_elapsed = end_cycle_count - last_cycle_count;
			int64_t counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
			AE_ASSERT(sizeof(float) == 4);
			float ms_per_frame_float = 1000.0f * (int32_t)counter_elapsed / (int32_t)perf_count_frequency;
			float frames_per_second = 1.0f / (int32_t)counter_elapsed * (int32_t)perf_count_frequency;
			float mega_cycles_per_frame = (int32_t)cycles_elapsed / (1000.0f * 1000.0f);

			AE_INFO("%.2f ms", ms_per_frame_float);
			AE_INFO("%.2f fps", frames_per_second);
			AE_INFO("%.2f million cycles / frame", mega_cycles_per_frame);

			last_counter = end_counter;
			last_cycle_count = end_cycle_count;
		}
	}

	void application_cleanup(RendererContext *r_context)
	{
		renderer_vulkan_cleanup(r_context);
	}
}