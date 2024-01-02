#pragma once
namespace AM_VkRenderCoreConstants
{
	constexpr static int MAX_FRAMES_IN_FLIGHT = 2;
	constexpr static const char* MODEL_PATH = "../data/models/vikingroom.obj";
	constexpr static const char* TEXTURE_PATH = "../data/textures/vikingroom.png";
	constexpr static uint64_t SINGLEALLOCSIZE = 0x4000000;
	constexpr static uint64_t UBO_ALIGNMENT = 0x200;
}