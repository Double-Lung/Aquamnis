#pragma once
#include "AM_AllocationObject.h"
#include "AM_VkImage.h"

class AM_Image : public AM_AllocationObject
{
public:
	AM_Image(AM_VkImage& anImage, const uint64_t anOffset, const uint64_t aSize)
		: AM_AllocationObject(AM_AllocationObject::IMAGE, anOffset, aSize)
		, myImage(std::move(anImage))
	{
	}

	AM_Image(const uint64_t anOffset, const uint64_t aSize)
		: AM_AllocationObject(AM_AllocationObject::IMAGE, anOffset, aSize)
		, myImage{}
	{
	}

	AM_Image(AM_Image&& anImage) noexcept
		: AM_AllocationObject(std::move(anImage))
	{
		*this = std::move(anImage);
	}

	~AM_Image() = default;

	void SetImage(AM_VkImage&& anImage) { myImage = std::move(anImage); }
	const AM_VkImage& GetImage() const { return myImage; }
	void Release() { myImage.Release(); }

private:
	AM_Image() = delete;
	AM_Image(const AM_Image& anImage) = delete; 
	AM_Image& operator=(const AM_Image& anImage) = delete;
	AM_Image& operator=(AM_Image&& anImage) noexcept
	{
		if (this == &anImage)
			return *this;

		myImage = std::move(anImage.myImage);
		return *this;
	}
	AM_VkImage myImage;
};
