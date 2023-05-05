#pragma once

// comment out below line to disable assertions
#define AE_ASSERTIONS_ENABLED

#ifdef AE_ASSERTIONS_ENABLED // all assertions

void assertion_print(const char *condition, const char *message, const char *file, int line);

#define AE_ASSERT(condition) \
	if (!(condition)) \
	{ \
		assertion_print(#condition, "", __FILE__, __LINE__); \
		__debugbreak(); \
		exit(1); \
	} 

#define AE_ASSERT_MSG(condition, message) \
	if (!(condition)) \
	{ \
		assertion_print(#condition, message, __FILE__, __LINE__); \
		__debugbreak(); \
		exit(1); \
	}

#ifdef _DEBUG // debug only assertions

#define AE_ASSERT_DEBUG(condition) \
	if (!(condition)) \
	{ \
		assertion_print(#condition, "", __FILE__, __LINE__); \
		__debugbreak(); \
		exit(1); \
	}

#define AE_ASSERT_DEBUG_MSG(condition, message) \
	if (!(condition)) \
	{ \
		assertion_print(#condition, message, __FILE__, __LINE__); \
		__debugbreak(); \
		exit(1); \
	}

#endif

#endif
