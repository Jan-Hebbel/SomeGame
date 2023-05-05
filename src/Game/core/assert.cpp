#include "aepch.hpp"

#include "assert.hpp"

#include "AnEngine/core/logger.hpp"

void assertion_print(const char *condition, const char *message, const char *file, int line)
{
	logging_log_output(AE_LOG_LEVEL_FATAL, "Assertion failure: %s, message: '%s', in file: %s, line: %d", condition, message, file, line);
}
