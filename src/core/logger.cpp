#include "core/logger.hpp"

#include "core/assert.hpp"

#include <iostream>
#include <Windows.h>

void logging_initialize()
{
	return;
}

void logging_shutdown()
{
	return;
}

void console_color_set(WORD *attributes, WORD color)
{
	CONSOLE_SCREEN_BUFFER_INFO info{};
	HANDLE h_console = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(h_console, &info);
	*attributes = info.wAttributes;
	SetConsoleTextAttribute(h_console, color);
}

void console_color_reset(WORD attributes)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attributes);
}

void logging_log_output(LogLevel log_level, const char *message, ...)
{
	const char *log_level_strings[] = { "[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[TRACE]: " };
	//bool is_error = log_level < 2;

	va_list arg_ptr;
	va_start(arg_ptr, message);
	int size = vsnprintf(NULL, 0, message, arg_ptr) + 1;
	char *out_message = new char[size];
	vsnprintf(out_message, size * sizeof(out_message[0]), message, arg_ptr);
	va_end(arg_ptr);

	WORD color;
	switch (log_level)
	{
		case 0: color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_RED; break;
		case 1: color = FOREGROUND_RED; break;
		case 2: color = FOREGROUND_RED | FOREGROUND_GREEN; break;
		case 3: color = FOREGROUND_GREEN | FOREGROUND_BLUE; break;
		case 4: color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
		default: AE_ASSERT_MSG(0, "Unknown log level!");
	}
	WORD attributes{};
	console_color_set(&attributes, color);
	std::cout << log_level_strings[log_level] << out_message;
	console_color_reset(attributes);
	std::cout << '\n';

	delete[] out_message;
}
