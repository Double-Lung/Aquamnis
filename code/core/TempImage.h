#pragma once
#include <vulkan/vulkan.h>

VK_DEFINE_HANDLE(VmaAllocation);

struct TempImage
{
	VkImage myImage{ nullptr };
	VmaAllocation myAllocation{ nullptr };
};