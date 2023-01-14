#pragma once
#include "AM_AllocationObject.h"

class AM_Image : public AM_AllocationObject
{
public:
	AM_Image()
		: AM_AllocationObject(AM_AllocationObject::IMAGE)
		, myImage(VK_NULL_HANDLE)
	{
	}

	AM_Image(const VkImage anImage, const uint64_t anOffset, const uint64_t aSize, const VkDeviceMemory aMemory)
		: AM_AllocationObject(AM_AllocationObject::IMAGE, anOffset, aSize, aMemory)
		, myImage(anImage)
	{
	}

	~AM_Image() = default;

	void Bind(const uint64_t anOffset, const uint64_t aSize, const VkImage anImage, const VkDeviceMemory aMemory)
	{
		AM_AllocationObject::Bind(anOffset, aSize, aMemory);
		myImage = anImage;
	}

	void Reset()
	{
		AM_AllocationObject::Reset();
		myImage = VK_NULL_HANDLE;
	}
private:
	VkImage myImage;
};
