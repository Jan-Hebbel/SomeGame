#include "platform/platform.hpp"

#include <xaudio2.h>

#include "AnEngine/core/logger.hpp"

/*
	TODO: NOT FINAL

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
	- ...
*/

struct PlatformWindow
{
	HINSTANCE hinstance;
	HWND hwnd;
	int width;
	int height;
};

// TODO: this is a global for now, later we will set this somewhere else
bool should_close = false;

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

HRESULT find_chunk(HANDLE hfile, DWORD fourcc, DWORD &dw_chunk_size, DWORD &dw_chunk_data_position)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hfile, 0, NULL, FILE_BEGIN))
	{
		AE_WARN("Couldn't set file pointer!");
		return HRESULT_FROM_WIN32(GetLastError());
	}

	DWORD dw_chunk_type;
	DWORD dw_chunk_data_size;
	DWORD dw_riff_data_size = 0;
	DWORD dw_file_type;
	DWORD bytes_read = 0;
	DWORD dw_offset = 0;

	while (hr == S_OK)
	{
		DWORD dw_read;
		if (ReadFile(hfile, &dw_chunk_type, sizeof(DWORD), &dw_read, NULL) == 0)
		{
			AE_WARN("Couldn't read audio file!");
			hr = HRESULT_FROM_WIN32(GetLastError());
		}

		if (ReadFile(hfile, &dw_chunk_data_size, sizeof(DWORD), &dw_read, NULL) == 0)
		{
			AE_WARN("Couldn't read audio file!");
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
					AE_WARN("Couldn't read audio file!");
					hr = HRESULT_FROM_WIN32(GetLastError());
				}
			} break;

			default:
			{
				if (INVALID_SET_FILE_POINTER == SetFilePointer(hfile, dw_chunk_data_size, NULL, FILE_CURRENT))
				{
					AE_WARN("Couldn't set file pointer!");
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

HRESULT read_chunk_data(HANDLE hfile, void *buffer, DWORD buffer_size, DWORD buffer_offset)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hfile, buffer_offset, NULL, FILE_BEGIN))
	{
		AE_WARN("Couldn't set file pointer!");
		return HRESULT_FROM_WIN32(GetLastError());
	}
	DWORD dw_read;
	if (ReadFile(hfile, buffer, buffer_size, &dw_read, NULL) == 0)
	{
		AE_WARN("Couldn't read file!");
		hr = HRESULT_FROM_WIN32(GetLastError());
	}
	return hr;
}

LRESULT CALLBACK
main_window_callback(HWND w_handle, UINT message, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;

	switch (message)
	{
		case WM_SIZE:
		{
			RECT client_rect{};
			GetClientRect(w_handle, &client_rect);
			// TODO: set window width and height somehow
			// TODO: signal window resize to vulkan renderer
		} break;

		case WM_DESTROY:
		{
			// TODO: handle this as an error - recreate window?
			should_close = true;
		} break;

		case WM_CLOSE:
		{
			// TODO: handle this as a message to the user?
			should_close = true;
		} break;

		case WM_ACTIVATEAPP:
		{
			// TODO: handle this
		} break;

		// TODO: set this up somewhere else
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

//float delta_time()
//{
//	LARGE_INTEGER begin_counter;
//	QueryPerformanceCounter(&begin_counter);
//
//	begin_counter.QuadPart = 
//}

void platform_create_window(const char *title, int width, int height, PlatformWindow *window)
{
	HINSTANCE instance = GetModuleHandleW(NULL);
	window->hinstance = instance;

	WNDCLASSA w_class{};
	w_class.style = CS_HREDRAW | CS_VREDRAW;
	w_class.lpfnWndProc = main_window_callback;
	w_class.hInstance = instance;
	//windowClass.hIcon = ;
	w_class.lpszClassName = "EngineWindowClass";

	if (!RegisterClassA(&w_class))
	{
		std::cerr << "Failed to register window class!" << '\n';
		exit(1);
	}

	HWND w_handle = CreateWindowExA(
		0,
		w_class.lpszClassName,
		title,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		0,
		0,
		instance,
		0);
	window->hwnd = w_handle;

	if (!w_handle)
	{
		std::cerr << "Failed to create window!" << '\n';
		exit(1);
	}


	// ------ initialize XAudio2 ------
	// initialize the COM library, sets the threads concurrency model
	HRESULT hresult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hresult))
	{
		AE_WARN("Failed to initialize the COM library: %lu; Proceeding without audio.", hresult);
		return;
	}

	// create an instance of the XAudio2 engine
	IXAudio2 *p_xaudio2 = NULL;
	hresult = XAudio2Create(&p_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hresult))
	{
		AE_WARN("Failed to create an instance of the XAudio2 engine: %lu; Proceeding without audio.", hresult);
		return;
	}

	// create mastering voice (sends audio data to hardware)
	IXAudio2MasteringVoice *p_mastering_voice = NULL;
	hresult = p_xaudio2->CreateMasteringVoice(&p_mastering_voice);
	if (FAILED(hresult))
	{
		AE_WARN("Failed to create mastering voice: %lu; Proceeding without audio.", hresult);
		return;
	}

	// populating XAudio2 structures with the contents of RIFF chunks
	WAVEFORMATEXTENSIBLE wfx{};
	XAUDIO2_BUFFER buffer{};

	// open the file
	const char *file = "../Sandbox/res/audio/test.wav";
	HANDLE hfile = CreateFileA(
		file,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);

	if (hfile == INVALID_HANDLE_VALUE)
	{
		AE_WARN("Couldn't find audio file %s!", file);
		return;
	}

	if (SetFilePointer(hfile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		AE_WARN("Couldn't set file pointer!");
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
		AE_WARN("Unexpected file type! Expected file type was fourccWave.");
		return;
	}

	// locate the 'fmt ' chunk and copy its contents into a WAVEFORMATEXTENSIBLE structure
	find_chunk(hfile, fourccFMT, dw_chunk_size, dw_chunk_position);
	read_chunk_data(hfile, &wfx, dw_chunk_size, dw_chunk_position);

	// locate the 'data' chunk and read its contents into a buffer
	// fill out the audio data buffer with the contents of the fourccDATA chunk
	find_chunk(hfile, fourccDATA, dw_chunk_size, dw_chunk_position);
	// TODO: delete this data from the heap
	BYTE *p_data_buffer = new BYTE[dw_chunk_size];
	read_chunk_data(hfile, p_data_buffer, dw_chunk_size, dw_chunk_position);

	// populate an XAUDIO2_BUFFER structure
	buffer.AudioBytes = dw_chunk_size; // size of the audio buffer in bytes
	buffer.pAudioData = p_data_buffer; // buffer containing audio data
	buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer 

	// start it playing
	IXAudio2SourceVoice *p_source_voice;

	hresult = p_xaudio2->CreateSourceVoice(&p_source_voice, (WAVEFORMATEX *)&wfx);
	if (FAILED(hresult))
	{
		AE_WARN("Failed to create source voice!");
		return;
	}

	hresult = p_source_voice->SubmitSourceBuffer(&buffer);
	if (FAILED(hresult))
	{
		AE_WARN("Failed to submit source buffer!");
		return;
	}

	hresult = p_source_voice->Start(0);
	if (FAILED(hresult))
	{
		AE_WARN("Couldn't play the sound!");
		return;
	}
	// --------------------------------
}

bool window_should_close()
{
	return should_close;
}

void process_events()
{
	MSG message;
	if (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}