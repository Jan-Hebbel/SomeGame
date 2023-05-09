#include "pch.hpp"

#include "platform/platform.hpp"

#include <windows.h>

#include <stdio.h>

void *output_handle = 0;

void platform_logging_init()
{
	AllocConsole();
	output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
}

void platform_log(const char *message, ...)
{
	va_list arg_ptr;
	va_start(arg_ptr, message);
	uint size = vsnprintf(0, 0, message, arg_ptr);
	char *out_message = new char[size];
	vsnprintf(out_message, size, message, arg_ptr);
	va_end(arg_ptr);

	WriteConsole(output_handle, out_message, size, 0, 0);
}

void platform_logging_free()
{
	output_handle = 0;
	FreeConsole();
}