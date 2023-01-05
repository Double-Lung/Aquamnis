#pragma once

#include <list>
#include <vector>
#include <vulkan/vulkan.h>
#include "VkDrawConstants.h"
#include "VkDrawContext.h"
#include <stdexcept>

struct AM_SimpleMemoryObject
{
	uint64_t myOffset = 0;
	uint64_t mySize = 0;
};

class AM_SimpleMemoryBlock
{
public:
	AM_SimpleMemoryBlock()
		: myMemory(nullptr)
		, myExtent(0)
	{
	}

	~AM_SimpleMemoryBlock() = default;
	AM_SimpleMemoryBlock(const AM_SimpleMemoryBlock&) = delete;
	void operator=(const AM_SimpleMemoryBlock&) = delete;

	AM_SimpleMemoryBlock(AM_SimpleMemoryBlock&& aMemoryBlock) noexcept
		: myMemory(nullptr)
		, myExtent(0)
	{
		*this = std::move(aMemoryBlock);
	}

	VkDeviceMemory myMemory;
	uint64_t myExtent;

private:
	AM_SimpleMemoryBlock& operator=(AM_SimpleMemoryBlock&& aMemoryBlock) noexcept
	{
		if (myMemory || this == &aMemoryBlock)
			return *this;

		myMemory = std::exchange(aMemoryBlock.myMemory, nullptr);
		myExtent = std::exchange(aMemoryBlock.myExtent, 0);
		return *this;
	}
};

class AM_NaiveMemoryAllocator
{
public:
	AM_NaiveMemoryAllocator() = default;
	~AM_NaiveMemoryAllocator()
	{
		FreeMemoryBlocks();
	};
	AM_NaiveMemoryAllocator(const AM_NaiveMemoryAllocator& anAllocator) = delete;
	AM_NaiveMemoryAllocator(const AM_NaiveMemoryAllocator&& anAllocator) = delete;

	void Initialize(uint32_t aMemoryTypeCount)
	{
		myMemoryBlocksByMemoryType.resize(aMemoryTypeCount);
		for (auto& memoryBlocks : myMemoryBlocksByMemoryType)
			memoryBlocks.reserve(64);
	}

	void Allocate(const uint32_t aMemoryTypeIndex, const uint64_t aSize)
	{
		auto& memoryBlock = GetMemoryBlock(aMemoryTypeIndex, aSize);
		SubAllocate(aSize);
	}

	void SubAllocate(const uint64_t aSize)
	{

	}

private:
	AM_SimpleMemoryBlock& CreateAndGetNewBlock(const uint32_t aMemoryTypeIndex)
	{
		auto& memoryBlocks = myMemoryBlocksByMemoryType[aMemoryTypeIndex];
		auto& block = memoryBlocks.emplace_back();

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = VkDrawConstants::SINGLEALLOCSIZE;
		allocInfo.memoryTypeIndex = aMemoryTypeIndex;

		if (vkAllocateMemory(VkDrawContext::device, &allocInfo, nullptr, &block.myMemory) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate memory of type ??? !");

		return block;
	}

	AM_SimpleMemoryBlock& GetMemoryBlock(const uint32_t aMemoryTypeIndex, const uint64_t aSize)
	{
		auto& memoryBlocks = myMemoryBlocksByMemoryType[aMemoryTypeIndex];

		// just grab the first suitable block
		for (auto& block : memoryBlocks)
		{
			if (block.myExtent + aSize <= VkDrawConstants::SINGLEALLOCSIZE)
				return block;
		}

		return CreateAndGetNewBlock(aMemoryTypeIndex);
	}

	void FreeMemoryBlocks()
	{
		for (auto& blocks : myMemoryBlocksByMemoryType)
		{
			for (auto& block : blocks)
			{
				vkFreeMemory(VkDrawContext::device, block.myMemory, nullptr);
			}
		}
	}
	std::vector<std::vector<AM_SimpleMemoryBlock>> myMemoryBlocksByMemoryType;
};

