#pragma once
#include <assert.h>
#include <list>
#include "AM_VkContext.h"

class AM_SimpleMemoryBlock
{
public:
	enum ResourceType
	{
		NOTSET,
		BUFFER,
		IMAGE
	};

	AM_SimpleMemoryBlock(const ResourceType aType)
		: myExtent(0)
		, myAlignment(0)
		, myUsage(0)
		, myType(aType)
		, myMemory(VK_NULL_HANDLE)
		, myMappedMemory(nullptr)
		, myIsMapped(false)
	{
	}

	AM_SimpleMemoryBlock(AM_SimpleMemoryBlock&& aMemoryBlock) noexcept
		: myExtent(0)
		, myAlignment(0)
		, myUsage(0)
		, myType(NOTSET)
		, myMemory(VK_NULL_HANDLE)
		, myMappedMemory(nullptr)
		, myIsMapped(false)
	{
		*this = std::move(aMemoryBlock);
	}

	virtual ~AM_SimpleMemoryBlock() { assert(!myMemory); }

	void Init(const uint32_t aMemoryTypeIndex, const uint64_t anAlignment)
	{
		myAlignment = anAlignment;
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = AM_VkRenderCoreConstants::SINGLEALLOCSIZE;
		allocInfo.memoryTypeIndex = aMemoryTypeIndex;
		if (vkAllocateMemory(myVkContext.device, &allocInfo, nullptr, &myMemory) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate memory of type ??? !");
	}

	template <typename T>
	T* Allocate(const uint64_t aSize, std::list<T>& anAllocationList);

	template <typename T>
	T* AllocateSlow(const uint64_t, std::list<T>& anAllocationList);

	uint64_t myExtent;
	uint64_t myAlignment;
	uint32_t myUsage;
	ResourceType myType;
	VkDeviceMemory myMemory;
	void* myMappedMemory;
	bool myIsMapped;

protected:
	virtual void* GetImageOrBufferHandle() const = 0;

private:
	AM_SimpleMemoryBlock() = delete;
	AM_SimpleMemoryBlock(const AM_SimpleMemoryBlock&) = delete;
	void operator=(const AM_SimpleMemoryBlock&) = delete;

	AM_SimpleMemoryBlock& operator=(AM_SimpleMemoryBlock&& aMemoryBlock) noexcept
	{
		if (this == &aMemoryBlock)
			return *this;
		myExtent = std::exchange(aMemoryBlock.myExtent, 0);
		myAlignment = std::exchange(aMemoryBlock.myAlignment, 0);
		myUsage = std::exchange(aMemoryBlock.myUsage, 0);
		myType = std::exchange(aMemoryBlock.myType, NOTSET);
		myMemory = std::exchange(aMemoryBlock.myMemory, nullptr);
		myMappedMemory = std::exchange(aMemoryBlock.myMappedMemory, nullptr);
		myIsMapped = std::exchange(aMemoryBlock.myIsMapped, false);
		return *this;
	}
};

template <typename T>
T* AM_SimpleMemoryBlock::Allocate(const uint64_t aSize, std::list<T>& anAllocationList)
{
	T& obj = anAllocationList.emplace_back(GetImageOrBufferHandle(), myExtent, aSize);
	obj.SetMemoryHandle(myMemory);
	obj.SetIsEmpty(false);
	myExtent += aSize;
	return &obj;
}

template <typename T>
T* AM_SimpleMemoryBlock::AllocateSlow(const uint64_t aSize, std::list<T>& anAllocationList)
{
	for (auto slotIter = anAllocationList.begin(); slotIter != anAllocationList.end(); ++slotIter)
	{
		if (!(slotIter->IsEmpty() && slotIter->GetSize() >= aSize))
			continue;

		const uint64_t leftover = slotIter->GetSize() - aSize;
		if (leftover == 0)
		{
			slotIter->SetIsEmpty(false);
			return &(*slotIter);
		}

		anAllocationList.emplace(slotIter, GetImageOrBufferHandle(), slotIter->GetOffset(), leftover);
		slotIter->SetOffset(slotIter->GetOffset() + leftover);
		slotIter->SetMemoryHandle(myMemory);
		slotIter->SetSize(aSize);
		slotIter->SetIsEmpty(false);
		return &(*slotIter);
	}

	return nullptr;
}
