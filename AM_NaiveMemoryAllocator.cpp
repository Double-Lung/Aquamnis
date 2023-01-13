#include "AM_NaiveMemoryAllocator.h"

void AM_NaiveMemoryAllocator::Init(uint32_t aMemoryTypeCount)
{
	myMemoryBlocksByMemoryType.resize(aMemoryTypeCount);
	for (auto& memoryBlocks : myMemoryBlocksByMemoryType)
		memoryBlocks.reserve(64);
}

AM_SimpleMemoryObject& AM_NaiveMemoryAllocator::Allocate(const uint32_t aMemoryTypeIndex, const VkMemoryRequirements& aMemoryRequirements)
{
	auto& memoryBlocks = myMemoryBlocksByMemoryType[aMemoryTypeIndex];
	for (auto& block : memoryBlocks)
	{
		uint64_t mod = block.myExtent % aMemoryRequirements.alignment;
		uint64_t padding = mod ? aMemoryRequirements.alignment - block.myExtent % aMemoryRequirements.alignment : 0;
		if (block.myExtent + padding + aMemoryRequirements.size <= VkDrawConstants::SINGLEALLOCSIZE)
		{
			if (padding)
			{
				block.myExtent += padding;
				block.myAllocations.back().mySize += padding;
			}
			auto& slot = block.myAllocations.emplace_back(block.myExtent, aMemoryRequirements.size, block.myMemory);
			block.myExtent += aMemoryRequirements.size;
			return slot;
		}

		if (auto* slot = TryGetFreeSlot(block, aMemoryRequirements))
			return *slot;
	}

	auto& newMemoryBlock = CreateAndGetNewBlock(aMemoryTypeIndex);
	newMemoryBlock.myExtent = aMemoryRequirements.size;
	return newMemoryBlock.myAllocations.emplace_back(0, aMemoryRequirements.size, newMemoryBlock.myMemory);
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

AM_SimpleMemoryObject* AM_NaiveMemoryAllocator::TryGetFreeSlot(AM_SimpleMemoryBlock& aMemoryBlock, const VkMemoryRequirements& aMemoryRequirements)
{
	for (auto slot = aMemoryBlock.myAllocations.begin(); slot != aMemoryBlock.myAllocations.end(); ++slot)
	{
		uint64_t mod = slot->myOffset % aMemoryRequirements.alignment;
		uint64_t padding = mod ? aMemoryRequirements.alignment - mod : 0;
		if (!(slot->myIsEmpty && slot->mySize >= aMemoryRequirements.size + padding))
			continue;

		const uint64_t leftover = slot->mySize - (aMemoryRequirements.size + padding);
		if (!(leftover || padding))
			return &(*slot);

		slot->mySize = aMemoryRequirements.size;
		if (padding)
		{
			slot->myOffset += padding;
			auto prevSlot = std::prev(slot);
			prevSlot->mySize += padding;
		}

		if (leftover)
			aMemoryBlock.myAllocations.emplace(std::next(slot), slot->myOffset + leftover, leftover, aMemoryBlock.myMemory);

		return &(*slot);
	}
	return nullptr;
}

void AM_NaiveMemoryAllocator::FreeVkDeviceMemory()
{
	for (auto& blocks : myMemoryBlocksByMemoryType)
	{
		for (auto& block : blocks)
		{
			vkFreeMemory(VkDrawContext::device, block.myMemory, nullptr);
			block.myMemory = nullptr;
		}
	}
}
