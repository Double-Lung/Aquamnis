#pragma once
#include "AM_Image.h"
#include "AM_SimpleMemoryBlock.h"
#include "AM_VkImage.h"

class AM_ImageMemoryBlock : public AM_SimpleMemoryBlock
{
public:
	AM_ImageMemoryBlock()
		: AM_SimpleMemoryBlock(AM_SimpleMemoryBlock::IMAGE)
	{
	}

	AM_ImageMemoryBlock(AM_ImageMemoryBlock&& aMemoryBlock) noexcept
		: AM_SimpleMemoryBlock(std::move(aMemoryBlock))
	{
		*this = std::move(aMemoryBlock);
	}

	~AM_ImageMemoryBlock()
	{
		myAllocationList.clear();
		if (myMemory)
		{
			vkFreeMemory(AM_VkContext::device, myMemory, nullptr);
			myMemory = nullptr;
		}
	}

	void Init(const uint32_t aMemoryTypeIndex, const uint64_t anAlignment)
	{
		myAlignment = anAlignment;
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = AM_VkRenderCoreConstants::SINGLEALLOCSIZE;
		allocInfo.memoryTypeIndex = aMemoryTypeIndex;
		if (vkAllocateMemory(AM_VkContext::device, &allocInfo, nullptr, &myMemory) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate memory of type ??? !");
	}

	AM_Image* Allocate(AM_VkImage& anImage, const uint64_t aSize)
	{
		AM_Image& image = myAllocationList.emplace_back(anImage, myExtent, aSize);
		vkBindImageMemory(AM_VkContext::device, image.GetImage().myImage, myMemory, myExtent);
		image.SetIsEmpty(false);
		myExtent += aSize;
		return &image;
	}

	std::list<AM_Image> myAllocationList;

private:
	AM_ImageMemoryBlock(const AM_ImageMemoryBlock& aMemoryBlock) = delete;
	AM_ImageMemoryBlock& operator=(const AM_ImageMemoryBlock& aMemoryBlock) = delete;
	AM_ImageMemoryBlock& operator=(AM_ImageMemoryBlock&& aMemoryBlock) noexcept
	{
		if (this == &aMemoryBlock)
			return *this;

		myAllocationList = std::move(aMemoryBlock.myAllocationList);
		return *this;
	}
};