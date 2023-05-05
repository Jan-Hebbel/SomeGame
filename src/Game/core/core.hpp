#pragma once

#ifdef AE_PLATFORM_WINDOWS
	#ifdef AE_BUILD_DLL
		#define AEAPI __declspec(dllexport)
	#else 
		#define AEAPI __declspec(dllimport)
	#endif
#else 
	#error AnEngine only supports Windows! (for now)
#endif 
