#pragma once

#include "AnEngine/core/core.hpp"

// functions for use in application

namespace AnEngine
{
	// to be defined in client application
	void placeholder();

	// to be called in client application
	AEAPI void application_create();
	AEAPI void application_run();
	AEAPI void application_cleanup();
}