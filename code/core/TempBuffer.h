#pragma once
#include <vulkan/vulkan.h>

struct VmaAllocator_T;
struct VmaAllocation_T;
typedef VmaAllocator_T* VmaAllocator;
typedef VmaAllocation_T* VmaAllocation;

struct TempBuffer
{
	VkBuffer myBuffer{ nullptr };
	VmaAllocation myAllocation{ nullptr };
};