#pragma once

#define VK_MAKE_VERSION(major, minor, patch) \
    ((((uint32_t)(major)) << 22U) | (((uint32_t)(minor)) << 12U) | ((uint32_t)(patch)))

namespace ApplicationConstants
{
	constexpr int MIN_WIDTH = 800;
	constexpr int MIN_HEIGHT = 600;
	constexpr int MAX_WIDTH = -1;
	constexpr int MAX_HEIGHT = -1;
	constexpr const char* WINDOWNAME = "Aquamnis";
	constexpr unsigned int APP_VERSION = ((((uint32_t)(0)) << 22U) | (((uint32_t)(1)) << 12U) | ((uint32_t)(0)));
}
