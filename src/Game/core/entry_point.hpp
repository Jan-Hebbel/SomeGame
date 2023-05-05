#pragma once

#include "AnEngine/core/application.hpp"

int main()
{
	AnEngine::application_create();
	AnEngine::application_run();
	AnEngine::application_cleanup();
}
