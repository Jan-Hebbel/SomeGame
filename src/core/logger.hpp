#pragma once

#if defined _DEBUG
	#define AE_LOG_ENABLE_WARN 1
	#define AE_LOG_ENABLE_INFO 1
	#define AE_LOG_ENABLE_TRACE 1
#elif defined NDEBUG
	#define AE_LOG_ENABLE_WARN 0
	#define AE_LOG_ENABLE_INFO 0
	#define AE_LOG_ENABLE_TRACE 0
#else 
	#error "Unknown Build Mode"
#endif

enum LogLevel
{
	AE_LOG_LEVEL_FATAL,
	AE_LOG_LEVEL_ERROR,
	AE_LOG_LEVEL_WARN,
	AE_LOG_LEVEL_INFO,
	AE_LOG_LEVEL_TRACE
};

void logging_initialize();
void logging_shutdown();

void logging_log_output(LogLevel log_level, const char *message, ...);

#ifndef AE_FATAL
#define AE_FATAL(message, ...) logging_log_output(AE_LOG_LEVEL_FATAL, message, __VA_ARGS__);
#endif

#ifndef AE_ERROR
#define AE_ERROR(message, ...) logging_log_output(AE_LOG_LEVEL_ERROR, message, __VA_ARGS__);
#endif

#if AE_LOG_ENABLE_WARN
#define AE_WARN(message, ...) logging_log_output(AE_LOG_LEVEL_WARN, message, __VA_ARGS__);
#else 
#define AE_WARN
#endif

#if AE_LOG_ENABLE_INFO
#define AE_INFO(message, ...) logging_log_output(AE_LOG_LEVEL_INFO, message, __VA_ARGS__);
#else 
#define AE_INFO
#endif

#if AE_LOG_ENABLE_TRACE
#define AE_TRACE(message, ...) logging_log_output(AE_LOG_LEVEL_TRACE, message, __VA_ARGS__);
#else 
#define AE_TRACE
#endif
