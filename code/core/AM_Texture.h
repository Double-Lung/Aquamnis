#pragma once
#include "TempImage.h"
#include <vulkan/vulkan.h>

struct AM_Texture
{
	TempImage myImage{};
	VkImageView myImageView{ nullptr };
	VkSampler mySampler{ nullptr };
	uint32_t myMipLevelCount{0};
};