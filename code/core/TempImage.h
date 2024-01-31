#pragma once
#include <vulkan/vulkan.h>

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

struct TempImage
{
	VkImage myImage{ nullptr };
	VmaAllocation myAllocation{ nullptr };
};