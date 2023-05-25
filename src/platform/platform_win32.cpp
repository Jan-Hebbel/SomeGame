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

#include "platform/platform.hpp"

#include "types.hpp"
#include "platform/platform_logger.hpp"
#include "game/game.hpp"

#include <windows.h>
#include <xaudio2.h>

#include <assert.h> // getchar()

struct Win32WindowHandles
{
	HINSTANCE hinstance;
	HWND hwnd;
};

struct Win32_Audio_Device
{
	IXAudio2 *xaudio2;
	BYTE *data_buffer;
};

class Voice_Callback : public IXAudio2VoiceCallback
{
public:
	HANDLE hBufferEndEvent;
	Voice_Callback() : hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL)) {}
	~Voice_Callback() { CloseHandle(hBufferEndEvent); }

	//Called when the voice has just finished playing a contiguous audio stream.
	void OnStreamEnd() { SetEvent(hBufferEndEvent); }

	//Unused methods are stubs
	void OnVoiceProcessingPassEnd() {}
	void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}
	void OnBufferEnd(void *pBufferContext) {
		delete[] pBufferContext;
	}
	void OnBufferStart(void *pBufferContext) {}
	void OnLoopEnd(void *pBufferContext) {}
	void OnVoiceError(void *pBufferContext, HRESULT Error) {}
};

enum Input_Events {
	WINDOW_CLOSE    = 0,
	PLAYER_FORWARD  = 1,
	PLAYER_LEFT     = 2,
	PLAYER_BACKWARD = 3,
	PLAYER_RIGHT    = 4,
};

constexpr uint WIDTH = 1440;
constexpr uint HEIGHT = 810;

global_variable bool should_close = true;
global_variable Win32_Audio_Device audio_device = {};
global_variable Win32WindowHandles window_handles = {};
global_variable Window_Dimensions window_dimensions = { WIDTH, HEIGHT };

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
		platform_log("Warning: Invalid set file pointer in find_chunk!\n");
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
			hr = HRESULT_FROM_WIN32(GetLastError());
		}

		if (ReadFile(hfile, &dw_chunk_data_size, sizeof(DWORD), &dw_read, NULL) == 0)
		{
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
					hr = HRESULT_FROM_WIN32(GetLastError());
				}
			} break;

			default:
			{
				if (INVALID_SET_FILE_POINTER == SetFilePointer(hfile, dw_chunk_data_size, NULL, FILE_CURRENT))
				{
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
		return HRESULT_FROM_WIN32(GetLastError());
	}
	DWORD dw_read;
	if (ReadFile(hfile, buffer, buffer_size, &dw_read, NULL) == 0)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}
	return hr;
}

internal_function bool32 platform_create_audio_device()
{
	// initialize the COM library, sets the threads concurrency model
	HRESULT hresult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hresult))
	{
		return GAME_FAILURE;
	}

	// create an instance of the XAudio2 engine
	hresult = XAudio2Create(&audio_device.xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hresult))
	{
		return GAME_FAILURE;
	}

	// create mastering voice (sends audio data to hardware)
	IXAudio2MasteringVoice *p_mastering_voice = NULL;
	hresult = audio_device.xaudio2->CreateMasteringVoice(&p_mastering_voice);
	if (FAILED(hresult))
	{
		return GAME_FAILURE;
	}

	return GAME_SUCCESS;
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
	audio_device.data_buffer = new BYTE[dw_chunk_size];
	read_chunk_data(hfile, audio_device.data_buffer, dw_chunk_size, dw_chunk_position);

	// populate an XAUDIO2_BUFFER structure
	buffer.AudioBytes = dw_chunk_size; // size of the audio buffer in bytes
	buffer.pAudioData = audio_device.data_buffer; // buffer containing audio data
	buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer 
	buffer.pContext = audio_device.data_buffer;

	// start it playing
	IXAudio2SourceVoice *p_source_voice;

	static Voice_Callback voice_callback;
	HRESULT hresult = audio_device.xaudio2->CreateSourceVoice(&p_source_voice, (WAVEFORMATEX *)&wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &voice_callback, NULL, NULL);
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

void platform_get_window_dimensions(Window_Dimensions *dimensions)
{
	dimensions->width = window_dimensions.width;
	dimensions->height = window_dimensions.height;
}

void platform_error_message_window(const char *title, const char *message)
{
	MessageBoxA(window_handles.hwnd, message, title, MB_OK | MB_ICONERROR);
}

LRESULT CALLBACK main_window_callback(HWND w_handle, UINT message, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;

	switch (message)
	{
		case WM_SIZE:
		{
			window_dimensions = { LOWORD(lparam), HIWORD(lparam) };
			//static int i = 0;
			//if (i > 0)
			//{
			//	if (wparam == SIZE_RESTORED) game_render(&game_state);
			//}
			//++i;
		} break;

		case WM_CLOSE:
		{
			// Todo: handle this as a message to the user?
			should_close = true;
		} break;

		case WM_ACTIVATEAPP:
		{
			// Todo: open menu?
		} break;

		case WM_DESTROY:
		{
			// Todo: handle this as an error - recreate window?
			should_close = true;
		} break;
		
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			assert(!"Keyboard Input came in through a dispatch message!");
		} break;

		default:
		{
			result = DefWindowProc(w_handle, message, wparam, lparam);
		} break;
	}

	return(result);
}

internal_function bool32 platform_create_window(const char *title, int width, int height)
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
		return GAME_FAILURE;
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
		return GAME_FAILURE;
	}

	window_handles.hwnd = w_handle;

	return GAME_SUCCESS;
}

internal_function void platform_process_events()
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) 
	{
		switch (message.message)
		{
			case WM_QUIT:
			{
				should_close = true;
			} break;

			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYUP:
			{
				WORD vk_code = LOWORD(message.wParam);
				WORD key_flags = HIWORD(message.lParam);

				bool released = (key_flags & KF_UP) == KF_UP;
				bool is_down = !released;
				bool repeated = ((key_flags & KF_REPEAT) == KF_REPEAT) && !released;
				bool alt_down = (key_flags & KF_ALTDOWN) == KF_ALTDOWN;

				// NOTE: holding down e.g. w and then while still holding w, holding down a will result in vkcode == a
				// to move at the same time with a and w, use: while (vkCode == 'W' && !released)
				// NOTE: to get only the first pressing of a button use: && is_down && !repeated
				if (vk_code == 'W')
				{
					OutputDebugStringA("W");
					if (released) OutputDebugStringA(" released");
					OutputDebugStringA("\n");
				}
				else if (vk_code == 'A')
				{

				}
				else if (vk_code == 'S')
				{

				}
				else if (vk_code == 'D')
				{
					OutputDebugStringA("D");
					if (released) OutputDebugStringA(" released");
					OutputDebugStringA("\n");
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

			default:
			{
				TranslateMessage(&message);
				DispatchMessage(&message);
			} break;
		}
	}
}

File_Asset *platform_read_file(const char *file_path, uint32 *bytes_read)
{
	// open file
	HANDLE file_handle = CreateFileA(file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file_handle == INVALID_HANDLE_VALUE) return NULL;

	// get file size
	LARGE_INTEGER large_integer = {};
	if (!GetFileSizeEx(file_handle, &large_integer)) {
		CloseHandle(file_handle);
		return NULL;
	}
	if (large_integer.u.HighPart > 0) { 
		CloseHandle(file_handle);
		return NULL; 
	}
	uint32 file_size = large_integer.u.LowPart;

	// allocate buffer
	File_Asset *file = new File_Asset[file_size];

	// read file to buffer
	uint32 number_of_bytes_read;
	if (!ReadFile(file_handle, file, file_size, (DWORD*)&number_of_bytes_read, NULL)) {
		delete[] file;
		CloseHandle(file_handle);
		return NULL;
	}

	if (bytes_read != NULL) *bytes_read = number_of_bytes_read;

	// close file
	CloseHandle(file_handle);

	return file;
}

void platform_free_file(File_Asset *file)
{
	delete[] file;
}

int CALLBACK WinMain(_In_ HINSTANCE h_instance, _In_opt_ HINSTANCE h_prev_instance, _In_ PSTR cmd_line, _In_ int cmdshow)
{
#ifdef _DEBUG
	platform_logging_init();
#endif

	//Game_Memory game_memory = {};
	//game_memory.permanent_storage_size = 64LL * 1024 * 1024;  // 64 MB
	//game_memory.transient_storage_size = 512LL * 1024 * 1024; // 512 MB
	//uint64 total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
	//game_memory.permanent_storage = VirtualAlloc(0, total_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	//game_memory.transient_storage = (uint8 *)game_memory.permanent_storage + game_memory.permanent_storage_size;

	LARGE_INTEGER perf_count_frequency_result;
	QueryPerformanceFrequency(&perf_count_frequency_result);
	int64 perf_count_frequency = perf_count_frequency_result.QuadPart;

	window_handles.hinstance = h_instance;

	bool32 result = platform_create_window("SomeGame", window_dimensions.width, window_dimensions.height);
	if (result != GAME_SUCCESS)
	{
		return result;
	}

	result = platform_create_audio_device();
	if (result != GAME_SUCCESS)
	{
		return result;
	}

	result = game_vulkan_init();
	if (result != GAME_SUCCESS)
	{
		platform_log("Fatal: Failed to initialize vulkan!\n");
		return result;
	}

	Game_State game_state = { /* Mode */ GAME_PLAY, /* Player {Position, Speed}*/ {{0.0f, 0.0f}, 5.0f}};
	
	should_close = false;

	LARGE_INTEGER last_counter;
	QueryPerformanceCounter(&last_counter);
	uint64 last_cycle_count = __rdtsc();

	while (!should_close)
	{
		// handle input and other events
		platform_process_events();

		//
		// Game Update and Render
		//
		game_update(&game_state, perf_metrics.ms_per_frame / 1000.0);
		game_render(&game_state);
		
		//
		// calculating performance metrics
		//
		uint64 end_cycle_count = __rdtsc();
		LARGE_INTEGER end_counter;
		QueryPerformanceCounter(&end_counter);

		uint64 cycles_elapsed = end_cycle_count - last_cycle_count;
		int64 counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
		perf_metrics.ms_per_frame = (1000.0f * (real64)counter_elapsed) / (real64)perf_count_frequency;
		perf_metrics.fps = (real64)perf_count_frequency / (real64)counter_elapsed;
		perf_metrics.mcpf = (real64)cycles_elapsed / (1000.0f * 1000.0f); // mcpf == mega cycles per frame

		last_counter = end_counter;
		last_cycle_count = end_cycle_count;
	}
	
	// don't crash on closing the application; TODO: is this needed?
	game_wait_idle();

	// cleanup
	//game_vulkan_cleanup();
	//platform_destroy_sound_device(&audio_device);
	//platform_destroy_window();

#ifdef _DEBUG
	platform_logging_free();
#endif
	
	return 0;
}