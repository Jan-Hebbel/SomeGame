/*
	TODO: NOT FINAL

	- logging
	- saved game locations
	- getting a handle to our own executable file
	- asset loading path
	- multithreading
	- raw input (multiple keyboards)
	- sleep / timebeginperiod
	- clipcursor() (multimonitor support)
	- fullscreen support
	- WM_SETCURSOR
	- querycancelautoplay
	- WM_ACTIVATEAPP
	- getkeyboard layouts
	- hardware acceleration
	- ...
*/

#include "pch.hpp"

#include "platform/platform.hpp"
#include "platform/platform_logger.hpp"

#include "game/game.hpp"

#include <windows.h>
#include <xaudio2.h>

struct Win32WindowHandles
{
	HINSTANCE hinstance;
	HWND hwnd;
};

struct Win32WindowDimensions
{
	uint width;
	uint height;
};

struct Win32SoundOutput
{
	IXAudio2 *p_xaudio2;
	BYTE *p_data_buffer;
};

constexpr uint WIDTH = 1440;
constexpr uint HEIGHT = 810;

global_variable bool32 should_close = 1;
global_variable Win32SoundOutput sound_device{};
global_variable Win32WindowHandles window_handles{};
global_variable Win32WindowDimensions window_dimensions = { WIDTH, HEIGHT };

// big endian
#ifdef _XBOX
#define fourccRIFF 'RIFF'
#define fourccDATA 'DATA'
#define fourccFMT  'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#else 
// little endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT  ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

internal_function HRESULT find_chunk(HANDLE hfile, DWORD fourcc, DWORD &dw_chunk_size, DWORD &dw_chunk_data_position)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hfile, 0, NULL, FILE_BEGIN))
	{
		//TODO: Logging
		return HRESULT_FROM_WIN32(GetLastError());
	}

	DWORD dw_chunk_type = 0;
	DWORD dw_chunk_data_size = 0;
	DWORD dw_riff_data_size = 0;
	DWORD dw_file_type = 0;
	DWORD bytes_read = 0;
	DWORD dw_offset = 0;

	while (hr == S_OK)
	{
		DWORD dw_read;
		if (ReadFile(hfile, &dw_chunk_type, sizeof(DWORD), &dw_read, NULL) == 0)
		{
			//TODO: Logging
			hr = HRESULT_FROM_WIN32(GetLastError());
		}

		if (ReadFile(hfile, &dw_chunk_data_size, sizeof(DWORD), &dw_read, NULL) == 0)
		{
			//TODO: Logging
			hr = HRESULT_FROM_WIN32(GetLastError());
		}

		switch (dw_chunk_type)
		{
			case fourccRIFF:
			{
				dw_riff_data_size = dw_chunk_data_size;
				dw_chunk_data_size = 4;
				if (ReadFile(hfile, &dw_file_type, sizeof(DWORD), &dw_read, NULL) == 0)
				{
					//TODO: Logging
					hr = HRESULT_FROM_WIN32(GetLastError());
				}
			} break;

			default:
			{
				if (INVALID_SET_FILE_POINTER == SetFilePointer(hfile, dw_chunk_data_size, NULL, FILE_CURRENT))
				{
					//TODO: Logging
					return HRESULT_FROM_WIN32(GetLastError());
				}
			}
		}

		dw_offset += sizeof(DWORD) * 2;

		if (dw_chunk_type == fourcc)
		{
			dw_chunk_size = dw_chunk_data_size;
			dw_chunk_data_position = dw_offset;
			return S_OK;
		}

		dw_offset += dw_chunk_data_size;

		if (bytes_read >= dw_riff_data_size) return S_FALSE;
	}

	return S_OK;
}

internal_function HRESULT read_chunk_data(HANDLE hfile, void *buffer, DWORD buffer_size, DWORD buffer_offset)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hfile, buffer_offset, NULL, FILE_BEGIN))
	{
		//TODO: Logging
		return HRESULT_FROM_WIN32(GetLastError());
	}
	DWORD dw_read;
	if (ReadFile(hfile, buffer, buffer_size, &dw_read, NULL) == 0)
	{
		//TODO: Logging
		hr = HRESULT_FROM_WIN32(GetLastError());
	}
	return hr;
}


internal_function void platform_create_sound_device()
{
	// ------ initialize XAudio2 ------
	// initialize the COM library, sets the threads concurrency model
	HRESULT hresult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hresult))
	{
		//TODO: Logging
		return;
	}

	// create an instance of the XAudio2 engine
	sound_device.p_xaudio2 = NULL;
	hresult = XAudio2Create(&sound_device.p_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hresult))
	{
		//TODO: Logging
		return;
	}

	// create mastering voice (sends audio data to hardware)
	IXAudio2MasteringVoice *p_mastering_voice = NULL;
	hresult = sound_device.p_xaudio2->CreateMasteringVoice(&p_mastering_voice);
	if (FAILED(hresult))
	{
		//TODO: Logging
		return;
	}
	// --------------------------------
}

internal_function void platform_destroy_sound_device()
{
	delete[] sound_device.p_data_buffer;
	// TODO: cleanup XAudio2 thingy?
}

void platform_audio_play_file(const char *file_path)
{
	// populating XAudio2 structures with the contents of RIFF chunks
	WAVEFORMATEXTENSIBLE wfx{};
	XAUDIO2_BUFFER buffer{};

	// open the file
	HANDLE hfile = CreateFileA(
		file_path,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);

	if (hfile == INVALID_HANDLE_VALUE)
	{
		//TODO: Logging
		return;
	}

	if (SetFilePointer(hfile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		//TODO: Logging
		return;
	}

	// locate the 'RIFF' chunk in the audio file, check the file type
	DWORD dw_chunk_size;
	DWORD dw_chunk_position;
	// check the file type, should be fourccWAVE or 'XWMA'
	find_chunk(hfile, fourccRIFF, dw_chunk_size, dw_chunk_position);
	DWORD file_type;
	read_chunk_data(hfile, &file_type, sizeof(DWORD), dw_chunk_position);
	if (file_type != fourccWAVE)
	{
		//TODO: Logging
		return;
	}

	// locate the 'fmt ' chunk and copy its contents into a WAVEFORMATEXTENSIBLE structure
	find_chunk(hfile, fourccFMT, dw_chunk_size, dw_chunk_position);
	read_chunk_data(hfile, &wfx, dw_chunk_size, dw_chunk_position);

	// locate the 'data' chunk and read its contents into a buffer
	// fill out the audio data buffer with the contents of the fourccDATA chunk
	find_chunk(hfile, fourccDATA, dw_chunk_size, dw_chunk_position);
	// TODO: delete this data from the heap
	sound_device.p_data_buffer = new BYTE[dw_chunk_size];
	read_chunk_data(hfile, sound_device.p_data_buffer, dw_chunk_size, dw_chunk_position);

	// populate an XAUDIO2_BUFFER structure
	buffer.AudioBytes = dw_chunk_size; // size of the audio buffer in bytes
	buffer.pAudioData = sound_device.p_data_buffer; // buffer containing audio data
	buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer 

	// start it playing
	IXAudio2SourceVoice *p_source_voice;

	HRESULT hresult = sound_device.p_xaudio2->CreateSourceVoice(&p_source_voice, (WAVEFORMATEX *)&wfx);
	if (FAILED(hresult))
	{
		//TODO: Logging
		return;
	}

	hresult = p_source_voice->SubmitSourceBuffer(&buffer);
	if (FAILED(hresult))
	{
		//TODO: Logging
		return;
	}

	hresult = p_source_voice->Start(0);
	if (FAILED(hresult))
	{
		//TODO: Logging
		return;
	}
}

void *platform_get_window_handles()
{
	return (void *)&window_handles;
}

void platform_get_window_dimensions(uint *width, uint *height)
{
	*width = window_dimensions.width;
	*height = window_dimensions.height;
}

void platform_error_message_window(const char *title, const char *message)
{
	MessageBoxA(window_handles.hwnd, message, title, MB_OK | MB_ICONERROR);
}

LRESULT CALLBACK
main_window_callback(HWND w_handle, UINT message, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;

	switch (message)
	{
		case WM_SIZE:
		{
			window_dimensions = { LOWORD(lparam), HIWORD(lparam) };
		} break;

		case WM_CLOSE:
		{
			// TODO: handle this as a message to the user?
			should_close = true;
		} break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_DESTROY:
		{
			// TODO: handle this as an error - recreate window?
			should_close = true;
		} break;
		
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			WORD vk_code = LOWORD(wparam);
			WORD key_flags = HIWORD(lparam);

			bool released = (key_flags & KF_UP) == KF_UP;
			bool is_down = !released;
			bool repeated = ((key_flags & KF_REPEAT) == KF_REPEAT) && !released;
			bool alt_down = (key_flags & KF_ALTDOWN) == KF_ALTDOWN;

			// NOTE: holding down e.g. w and then while still holding w, holding down a will result in vkcode == a
			// to move at the same time with a and w, use: while (vkCode == 'W' && !released)
			// NOTE: to get only the first pressing of a button use: && is_down && !repeated
			if (vk_code == 'W')
			{
				
			}
			else if (vk_code == 'A')
			{

			}
			else if (vk_code == 'S')
			{

			}
			else if (vk_code == 'D')
			{

			}
			else if (vk_code == 'Q')
			{

			}
			else if (vk_code == 'E')
			{

			}
			else if (vk_code == 'R')
			{

			}
			else if (vk_code == VK_LEFT)
			{

			}
			else if (vk_code == VK_UP)
			{

			}
			else if (vk_code == VK_RIGHT)
			{

			}
			else if (vk_code == VK_DOWN)
			{

			}
			else if (vk_code == VK_ESCAPE)
			{

			}
			else if (vk_code == VK_SPACE)
			{

			}
			else if (vk_code == VK_F4 && alt_down)
			{
				should_close = true;
			}
		} break;


		case WM_PAINT:
		{
			// TODO: delete this once vulkan renderer set up
			PAINTSTRUCT paint{};
			HDC device_context = BeginPaint(w_handle, &paint);
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;
			int width = paint.rcPaint.right - paint.rcPaint.left;
			int height = paint.rcPaint.bottom - paint.rcPaint.top;
			PatBlt(device_context, x, y, width, height, WHITENESS);
			EndPaint(w_handle, &paint);
		} break;

		default:
		{
			result = DefWindowProc(w_handle, message, wparam, lparam);
		} break;
	}

	return(result);
}

// TODO: bool32 return type; not exiting in this function
internal_function void platform_create_window(const char *title, int width, int height)
{	
	WNDCLASSA w_class{};
	w_class.style = CS_HREDRAW | CS_VREDRAW;
	w_class.lpfnWndProc = main_window_callback;
	w_class.hInstance = window_handles.hinstance;
	w_class.hCursor = LoadCursor(0, IDC_ARROW);
	//windowClass.hIcon = ;
	w_class.lpszClassName = "EngineWindowClass";

	if (!RegisterClassA(&w_class))
	{
		//TODO: Logging
		exit(1);
	}

	HWND w_handle = CreateWindowExA(
		0,
		w_class.lpszClassName,
		title,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		100, 
		100,
		width, 
		height,
		0,
		0,
		w_class.hInstance,
		0);

	if (!w_handle)
	{
		//TODO: Logging
		exit(1);
	}

	// TODO: Logging (successfully created window)
	window_handles.hwnd = w_handle;
}

internal_function void platform_process_events()
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		if (message.message == WM_QUIT)
		{
			should_close = true;
		}

		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}

int CALLBACK WinMain(_In_ HINSTANCE h_instance, _In_opt_ HINSTANCE h_prev_instance, _In_ PSTR cmd_line, _In_ int cmdshow)
{
	platform_logging_init();

	LARGE_INTEGER perf_count_frequency_result;
	QueryPerformanceFrequency(&perf_count_frequency_result);
	int64 perf_count_frequency = perf_count_frequency_result.QuadPart;

	window_handles.hinstance = h_instance;

	platform_create_window("SomeGame", window_dimensions.width, window_dimensions.height);
	platform_create_sound_device();
	// TODO: remove this
	platform_audio_play_file("res/audio/test.wav");

	bool32 result_vulkan_init = game_vulkan_init();
	if (result_vulkan_init == GAME_FAILURE)
	{
		exit(1);
	}
	
	should_close = 0;

	LARGE_INTEGER last_counter;
	QueryPerformanceCounter(&last_counter);
	uint64 last_cycle_count = __rdtsc();

	while (!should_close)
	{
		// handle input and other events
		platform_process_events();
		
		// update game and render
		game_update();
		if (window_dimensions.width != 0 && window_dimensions.height != 0)
		{
			game_render();
		}
		
		// function: calculate performance metrics
		uint64 end_cycle_count = __rdtsc();
		LARGE_INTEGER end_counter;
		QueryPerformanceCounter(&end_counter);

		uint64 cycles_elapsed = end_cycle_count - last_cycle_count;
		int64 counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
		real64 ms_per_frame = (1000.0f * (real64)counter_elapsed) / (real64)perf_count_frequency;
		real64 fps = (real64)perf_count_frequency / (real64)counter_elapsed;
		// mcpf == mega cycles per frame
		real64 mcpf = (real64)cycles_elapsed / (1000.0f * 1000.0f);

		// TODO: display metrics to screen

		last_counter = end_counter;
		last_cycle_count = end_cycle_count;
		// end of calculating performance metrics
	}
	
	// don't crash on closing the application; TODO: is this needed?
	game_wait_idle();

	// cleanup
	//game_vulkan_cleanup();
	platform_destroy_sound_device();
	//platform_destroy_window(); ????

	platform_logging_free();
	
	return 0;
}