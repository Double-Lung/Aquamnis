#pragma once
#include "TempImage.h"
#include <vulkan/vulkan.h>

class AM_Texture
{
public:
	explicit AM_Texture(uint32_t aMipLevelCount = 0)
		: myImage{}
		, myImageView(nullptr)
		, mySampler(nullptr)
		, myMipLevelCount(aMipLevelCount)
	{
	}

	TempImage myImage;
	VkImageView myImageView;
	VkSampler mySampler;
	uint32_t myMipLevelCount;
};