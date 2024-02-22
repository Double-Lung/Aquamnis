#pragma once
#include <vulkan/vulkan.h>

VK_DEFINE_HANDLE(VmaAllocation);
struct TempBuffer
{
	VkBuffer myBuffer{ nullptr };
	VmaAllocation myAllocation{ nullptr };
};