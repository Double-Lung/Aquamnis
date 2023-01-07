#include "AM_NaiveMemoryAllocator.h"
#include "VkDrawConstants.h"
#include "VkDrawContext.h"
#include <stdexcept>

AM_NaiveMemoryAllocator::~AM_NaiveMemoryAllocator()
{
	FreeMemoryBlocks();
}

void AM_NaiveMemoryAllocator::Init(uint32_t aMemoryTypeCount)
{
	myMemoryBlocksByMemoryType.resize(aMemoryTypeCount);
	for (auto& memoryBlocks : myMemoryBlocksByMemoryType)
		memoryBlocks.reserve(64);
}

AM_SimpleMemoryObject& AM_NaiveMemoryAllocator::Allocate(const uint32_t aMemoryTypeIndex, const uint64_t aSize)
{
	auto& memoryBlock = GetMemoryBlock(aMemoryTypeIndex, aSize);
	return SubAllocate(memoryBlock, aSize);
}

AM_SimpleMemoryBlock& AM_NaiveMemoryAllocator::CreateAndGetNewBlock(const uint32_t aMemoryTypeIndex)
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

AM_SimpleMemoryBlock& AM_NaiveMemoryAllocator::GetMemoryBlock(const uint32_t aMemoryTypeIndex, const uint64_t aSize)
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

AM_SimpleMemoryObject& AM_NaiveMemoryAllocator::SubAllocate(AM_SimpleMemoryBlock& aMemoryBlock, const uint64_t aSize)
{
	for (auto begin = aMemoryBlock.myAllocations.begin(); begin != aMemoryBlock.myAllocations.end(); ++begin)
	{
		if (!(begin->myIsEmpty && begin->mySize >= aSize))
			continue;

		const uint64_t leftover = begin->mySize - aSize;
		if (!leftover)
			return *begin;

		aMemoryBlock.myAllocations.emplace(begin, begin->myOffset, leftover);
		begin->mySize = aSize;
		begin->myOffset += leftover;
		return *begin;
	}

	return aMemoryBlock.myAllocations.emplace_back(aMemoryBlock.myExtent, aSize);
}

void AM_NaiveMemoryAllocator::FreeMemoryBlocks()
{
	for (auto& blocks : myMemoryBlocksByMemoryType)
	{
		for (auto& block : blocks)
		{
			vkFreeMemory(VkDrawContext::device, block.myMemory, nullptr);
		}
	}
}
