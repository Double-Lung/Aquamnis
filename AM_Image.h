#pragma once
#include "AM_AllocationObject.h"

class AM_Image : public AM_AllocationObject
{
public:
	

	AM_Image(const VkImage anImage, const uint64_t anOffset, const uint64_t aSize)
		: AM_AllocationObject(AM_AllocationObject::IMAGE, anOffset, aSize)
		, myImage(anImage)
	{
	}

	~AM_Image() = default;

	void Reset()
	{
		AM_AllocationObject::Reset();
		myImage = VK_NULL_HANDLE;
	}
private:
	AM_Image() = delete;
	AM_Image(const AM_Image& anImage) = delete; 
	AM_Image& operator=(const AM_Image& anImage) = delete;
	VkImage myImage;
};
