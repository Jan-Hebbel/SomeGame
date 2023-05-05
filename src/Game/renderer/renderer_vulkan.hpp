#pragma once

#include "platform/platform.hpp"

#ifdef AE_BUILD_RELEASE
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

void renderer_vulkan_init(PlatformWindow *window);
void renderer_vulkan_cleanup();