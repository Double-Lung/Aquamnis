#pragma once
namespace AM_VkRenderCoreConstants
{
	constexpr static int MAX_FRAMES_IN_FLIGHT = 2;
	constexpr static const char* MODEL_PATH = "../data/models/vikingroom.obj";
	constexpr static const char* TEXTURE_PATH = "../data/textures/vikingroom.png";
	constexpr static uint64_t SINGLEALLOCSIZE = 0x4000000;
	constexpr static uint64_t UBO_ALIGNMENT = 0x200;

	constexpr static const char* CUBEMAP_TEXTURE_PATH[6] =
	{
		"../data/textures/cubemaps/Yokohama/posx.jpg",
		"../data/textures/cubemaps/Yokohama/posy.jpg",
		"../data/textures/cubemaps/Yokohama/posz.jpg",
		"../data/textures/cubemaps/Yokohama/negx.jpg",
		"../data/textures/cubemaps/Yokohama/negy.jpg",
		"../data/textures/cubemaps/Yokohama/negz.jpg"
	};
}