#include "AM_NaiveMemoryAllocator.h"
#include <assert.h>

void AM_NaiveMemoryAllocator::Init(const VkPhysicalDeviceMemoryProperties& aPhysicalMemoryProperties)
{
	myPhysicalMemoryProperties = aPhysicalMemoryProperties;
	myMemoryBlocksByMemoryType.resize(myPhysicalMemoryProperties.memoryTypeCount);
	for (auto& memoryBlocks : myMemoryBlocksByMemoryType)
		memoryBlocks.reserve(32);

	//////////////////////////////////////////////

	for (uint32_t i = 0; i < myPhysicalMemoryProperties.memoryTypeCount; ++i)
	{
		auto& bufferMemPool = myBufferMemoryPool[i];
		bufferMemPool.reserve(32);

		auto& imageMemPool = myImageMemoryPool[i];
		imageMemPool.reserve(32);
	}
}

AM_SimpleMemoryObject& AM_NaiveMemoryAllocator::Allocate(const uint32_t aMemoryTypeIndex, const VkMemoryRequirements& aMemoryRequirements)
{
	auto& memoryBlocks = myMemoryBlocksByMemoryType[aMemoryTypeIndex];
	for (auto& block : memoryBlocks)
		if (auto* slot = SubAllocation(block, aMemoryRequirements))
			return *slot;

	auto& newMemoryBlock = memoryBlocks.emplace_back();
	AllocateMemory(newMemoryBlock.myMemory, aMemoryTypeIndex);
	newMemoryBlock.myExtent = aMemoryRequirements.size;
	return newMemoryBlock.myAllocations.emplace_back(0, aMemoryRequirements.size, newMemoryBlock.myMemory);
}

void AM_NaiveMemoryAllocator::AllocateMemory(VkDeviceMemory& outMemoryPtr, const uint32_t aMemoryTypeIndex)
{
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = VkDrawConstants::SINGLEALLOCSIZE;
	allocInfo.memoryTypeIndex = aMemoryTypeIndex;
	if (vkAllocateMemory(VkDrawContext::device, &allocInfo, nullptr, &outMemoryPtr) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate memory of type ??? !");
}

AM_SimpleMemoryObject& AM_NaiveMemoryAllocator::AllocateMappedBufferMemory(void** outMappedMemory, AM_SimpleMemoryBlock& aMemoryBlock, const uint32_t aMemoryTypeIndex, const VkMemoryRequirements& aMemoryRequirements)
{
	if (!aMemoryBlock.myMemory)
	{
		AllocateMemory(aMemoryBlock.myMemory, aMemoryTypeIndex);
		aMemoryBlock.myExtent = aMemoryRequirements.size;
		vkMapMemory(VkDrawContext::device, aMemoryBlock.myMemory, 0, VkDrawConstants::SINGLEALLOCSIZE, 0, &aMemoryBlock.myMappedMemory);
		*outMappedMemory = aMemoryBlock.myMappedMemory;
		return aMemoryBlock.myAllocations.emplace_back(0, aMemoryRequirements.size, aMemoryBlock.myMemory);
	}

	auto* slot = SubAllocation(aMemoryBlock, aMemoryRequirements);
	if (!slot)
		throw std::runtime_error("Failed to allocate mapped buffer memory!");
	*outMappedMemory = (char*)aMemoryBlock.myMappedMemory;
	return *slot;
}

AM_SimpleMemoryObject* AM_NaiveMemoryAllocator::SubAllocation(AM_SimpleMemoryBlock& aMemoryBlock, const VkMemoryRequirements& aMemoryRequirements)
{
	assert(aMemoryBlock.myMemory && "Allocation in an empty memory block!");
	uint64_t padding = GetPadding(aMemoryBlock.myExtent, aMemoryRequirements.alignment);
	if (aMemoryBlock.myExtent + padding + aMemoryRequirements.size <= VkDrawConstants::SINGLEALLOCSIZE)
	{
		if (padding)
		{
			aMemoryBlock.myExtent += padding;
			aMemoryBlock.myAllocations.back().mySize += padding;
		}
		auto& slot = aMemoryBlock.myAllocations.emplace_back(aMemoryBlock.myExtent, aMemoryRequirements.size, aMemoryBlock.myMemory);
		aMemoryBlock.myExtent += aMemoryRequirements.size;
		return &slot;
	}
	auto* slot = TryGetFreeSlot(aMemoryBlock, aMemoryRequirements);
	return slot;
}

uint64_t AM_NaiveMemoryAllocator::GetPadding(const uint64_t anOffset, const uint64_t anAlignmentSize) const 
{
	uint64_t mod = anOffset % anAlignmentSize;
	return (mod ? anAlignmentSize - mod : 0);
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

uint32_t AM_NaiveMemoryAllocator::FindMemoryTypeIndex(const uint32_t memoryTypeBits, VkMemoryPropertyFlags const properties) const
{
	const VkMemoryType* memoryTypes = myPhysicalMemoryProperties.memoryTypes;
	for (uint32_t i = 0; i < myPhysicalMemoryProperties.memoryTypeCount; ++i)
		if ((memoryTypeBits & (1 << i)) && (memoryTypes[i].propertyFlags == properties))
			return i;

	throw std::runtime_error("failed to find suitable memory type!");
}

AM_Buffer* AM_NaiveMemoryAllocator::AllocateBuffer(const uint64_t aSize, VkBufferUsageFlags aUsage)
{
	auto it = myBufferUsageToMemTypeIndex.find(aUsage);
	if (it != myBufferUsageToMemTypeIndex.cend())
	{
		AM_Buffer* buffer = nullptr;
		uint32_t memoryTypeIndex = it->second;
		if (buffer = AllocateBufferFast(aSize, memoryTypeIndex))
			return buffer;
		if (buffer = AllocateBufferSlow(aSize, memoryTypeIndex))
			return buffer;
	}

	return AllocateBufferWithNewBlock(aSize, aUsage);
}

AM_Buffer* AM_NaiveMemoryAllocator::AllocateMappedBuffer(const uint64_t aSize, VkBufferUsageFlags aUsage)
{
	return nullptr;
}

AM_Buffer* AM_NaiveMemoryAllocator::AllocateBufferWithNewBlock(const uint64_t aSize, const VkBufferUsageFlags aUsage)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = aSize;
	bufferInfo.usage = aUsage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	AM_VkBuffer vkBuffer(bufferInfo);
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(VkDrawContext::device, vkBuffer.myBuffer, &memRequirements);
	uint32_t memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	myBufferUsageToMemTypeIndex[aUsage] = memoryTypeIndex;

	std::vector<AM_BufferMemoryBlock>& bufferMemPool = myBufferMemoryPool[memoryTypeIndex];
	AM_BufferMemoryBlock& newBlock = bufferMemPool.emplace_back();
	newBlock.SetBuffer(vkBuffer);
	newBlock.BindBufferMemory(memoryTypeIndex);

	return newBlock.Allocate(aSize);
}

AM_Buffer* AM_NaiveMemoryAllocator::AllocateBufferFast(const uint64_t aSize, const uint32_t aMemoryTypeIndex)
{
	std::vector<AM_BufferMemoryBlock>& bufferMemPool = myBufferMemoryPool[aMemoryTypeIndex];

	for (AM_BufferMemoryBlock& block : bufferMemPool)
	{
		if (block.myExtent + aSize > VkDrawConstants::SINGLEALLOCSIZE)
			continue;

		return block.Allocate(aSize);
	}
	return nullptr;
}

AM_Buffer* AM_NaiveMemoryAllocator::AllocateBufferSlow(const uint64_t aSize, const uint32_t aMemoryTypeIndex)
{
	std::vector<AM_BufferMemoryBlock>& bufferMemPool = myBufferMemoryPool[aMemoryTypeIndex];

	for (AM_BufferMemoryBlock& block : bufferMemPool)
	{
		if (!block.HasFreeSlot())
			continue;

		for (auto it = block.myAllocationList.begin(); it != block.myAllocationList.end(); ++it)
		{
			if (!(it->IsEmpty() && it->GetSize() >= aSize))
				continue;

			const uint64_t leftover = it->GetSize() - aSize;
			if (leftover == 0)
				return &(*it);

			block.myAllocationList.emplace(it, block.myBuffer.myBuffer, it->GetOffset(), leftover, block.myMemory);
			it->SetOffset(it->GetOffset() + leftover);
			it->SetSize(aSize);
			return &(*it);
		}
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
	vkFreeMemory(VkDrawContext::device, myUniformBufferMemoryBlock.myMemory, nullptr);
	vkFreeMemory(VkDrawContext::device, myStagingBufferMemoryBlock.myMemory, nullptr);
	myUniformBufferMemoryBlock.myMemory = nullptr;
	myStagingBufferMemoryBlock.myMemory = nullptr;
}
