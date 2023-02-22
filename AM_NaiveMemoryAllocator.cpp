#include "AM_NaiveMemoryAllocator.h"
#include <assert.h>

void AM_NaiveMemoryAllocator::Init(const VkPhysicalDeviceMemoryProperties& aPhysicalMemoryProperties)
{
	
	myMemoryBlocksByMemoryType.resize(aPhysicalMemoryProperties.memoryTypeCount);
	for (auto& memoryBlocks : myMemoryBlocksByMemoryType)
		memoryBlocks.reserve(8);

	//////////////////////////////////////////////

	
	std::unordered_set<VkFlags> memoryProperties;
	myPhysicalMemoryProperties = aPhysicalMemoryProperties;

	myBufferMemoryPool.reserve(aPhysicalMemoryProperties.memoryTypeCount);
	myImageMemoryPool.reserve(aPhysicalMemoryProperties.memoryTypeCount);
	for (uint32_t i = 0; i < myPhysicalMemoryProperties.memoryTypeCount; ++i)
	{
		myBufferMemoryPool.emplace_back().reserve(8);
		myImageMemoryPool.emplace_back().reserve(8);
		memoryProperties.insert(myPhysicalMemoryProperties.memoryTypes[i].propertyFlags);
	}

	myMemPropCache.reserve(memoryProperties.size());
	for (auto it = memoryProperties.begin(); it != memoryProperties.cend(); ++it)
	{
		MemoryPropertyCache& cache = myMemPropCache.emplace_back();
		cache.myMemoryProperty = *it;
		cache.myMemReqByBufferUsage.reserve(8);
	}
}

AM_SimpleMemoryObject& AM_NaiveMemoryAllocator::Allocate(const uint32_t aMemoryTypeIndex, const VkMemoryRequirements& aMemoryRequirements)
{
	auto& memoryBlocks = myMemoryBlocksByMemoryType[aMemoryTypeIndex];
	for (auto& block : memoryBlocks)
		if (auto* slot = SubAllocation(block, aMemoryRequirements))
			return *slot;

	auto& newMemoryBlock = memoryBlocks.emplace_back(AM_SimpleMemoryBlock::BUFFER);
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

uint32_t AM_NaiveMemoryAllocator::FindMemoryTypeIndex(const uint32_t someMemoryTypeBits, const VkMemoryPropertyFlags someProperties) const
{
	const VkMemoryType* memoryTypes = myPhysicalMemoryProperties.memoryTypes;
	for (uint32_t i = 0; i < myPhysicalMemoryProperties.memoryTypeCount; ++i)
		if ((someMemoryTypeBits & (1 << i)) && (memoryTypes[i].propertyFlags == someProperties))
			return i;

	throw std::runtime_error("failed to find suitable memory type!");
}

AM_Buffer* AM_NaiveMemoryAllocator::AllocateBuffer(const uint64_t aSize, const VkBufferUsageFlags aUsage, const VkMemoryPropertyFlags aProperty)
{
	const MemoryRequirements* memReq = nullptr;
	auto it = myMemPropCache.begin();
	for (it; it != myMemPropCache.cend(); ++it)
	{
		if (it->myMemoryProperty != aProperty)
			continue;

		auto it2 = it->myMemReqByBufferUsage.find(aUsage);
		if (it2 != it->myMemReqByBufferUsage.cend())
			memReq = &((*it2).second);
		break;
	}

	assert(it != myMemPropCache.cend() && "Invalid VkMemoryPropertyFlags!");

	if (memReq)
	{
		if (AM_Buffer* buffer = AllocateBufferFast(aSize, *memReq))
			return buffer;
		if (AM_Buffer* buffer = AllocateBufferSlow(aSize, *memReq))
			return buffer;
	}

	return AllocateBufferWithNewBlock(aSize, aUsage, aProperty, *it);
}

AM_Buffer* AM_NaiveMemoryAllocator::AllocateMappedBuffer(const uint64_t aSize, const VkBufferUsageFlags aUsage)
{
	VkMemoryPropertyFlags memProp = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	const MemoryRequirements* memReq = nullptr;
	auto it = myMemPropCache.begin();
	for (it; it != myMemPropCache.cend(); ++it)
	{
		if (it->myMemoryProperty != memProp)
			continue;

		auto it2 = it->myMemReqByBufferUsage.find(aUsage);
		if (it2 != it->myMemReqByBufferUsage.cend())
			memReq = &((*it2).second);
		break;
	}

	assert(it != myMemPropCache.cend() && "Invalid VkMemoryPropertyFlags!");

	if (memReq)
	{
		if (AM_Buffer* buffer = AllocateMappedBufferFast(aSize, *memReq))
			return buffer;
		if (AM_Buffer* buffer = AllocateMappedBufferSlow(aSize, *memReq))
			return buffer;
	}

	return AllocateMappedBufferWithNewBlock(aSize, aUsage, memProp, *it);
}

AM_Buffer* AM_NaiveMemoryAllocator::AllocateBufferWithNewBlock(const uint64_t aSize, const VkBufferUsageFlags aUsage, const VkMemoryPropertyFlags aProperty, MemoryPropertyCache& aCache)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = VkDrawConstants::SINGLEALLOCSIZE;
	bufferInfo.usage = aUsage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	AM_VkBuffer vkBuffer(bufferInfo);
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(VkDrawContext::device, vkBuffer.myBuffer, &memRequirements);
	uint32_t memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, aProperty);
	MemoryRequirements& req = aCache.myMemReqByBufferUsage[aUsage];
	req.myAlignment = memRequirements.alignment;
	req.myMemoryTypeIndex = memoryTypeIndex;

	std::vector<AM_BufferMemoryBlock>& bufferMemPool = myBufferMemoryPool[memoryTypeIndex];
	AM_BufferMemoryBlock& newBlock = bufferMemPool.emplace_back();
	newBlock.Init(vkBuffer, memoryTypeIndex, req.myAlignment);
	return newBlock.Allocate(aSize + (newBlock.myAlignment - (aSize % newBlock.myAlignment)));
}

AM_Buffer* AM_NaiveMemoryAllocator::AllocateBufferFast(const uint64_t aSize, const MemoryRequirements& aRequirement)
{
	std::vector<AM_BufferMemoryBlock>& bufferMemPool = myBufferMemoryPool[aRequirement.myMemoryTypeIndex];
	for (AM_BufferMemoryBlock& block : bufferMemPool)
	{
		if (block.myAlignment != aRequirement.myAlignment)
			continue;

		const uint64_t extraBytes = aSize % block.myAlignment;
		const uint64_t paddedSize = extraBytes ? aSize + block.myAlignment - extraBytes : aSize;
		if (block.myExtent + paddedSize > VkDrawConstants::SINGLEALLOCSIZE)
			continue;

		return block.Allocate(paddedSize);
	}
	return nullptr;
}

AM_Buffer* AM_NaiveMemoryAllocator::AllocateBufferSlow(const uint64_t aSize, const MemoryRequirements& aRequirement)
{
	std::vector<AM_BufferMemoryBlock>& bufferMemPool = myBufferMemoryPool[aRequirement.myMemoryTypeIndex];
	for (AM_BufferMemoryBlock& block : bufferMemPool)
	{
		if (block.myAlignment != aRequirement.myAlignment)
			continue;

		for (auto slotIter = block.myAllocationList.begin(); slotIter != block.myAllocationList.end(); ++slotIter)
		{
			const uint64_t extraBytes = aSize % block.myAlignment;
			const uint64_t paddedSize = extraBytes ? aSize + block.myAlignment - extraBytes : aSize;
			if (!(slotIter->IsEmpty() && slotIter->GetSize() >= paddedSize))
				continue;

			const uint64_t leftover = slotIter->GetSize() - paddedSize;
			if (leftover == 0)
				return &(*slotIter);

			block.myAllocationList.emplace(slotIter, block.myBuffer.myBuffer, slotIter->GetOffset(), leftover);
			slotIter->SetOffset(slotIter->GetOffset() + leftover);
			slotIter->SetSize(paddedSize);
			slotIter->SetIsEmpty(false);
			return &(*slotIter);
		}
	}

	return nullptr;
}

AM_Buffer* AM_NaiveMemoryAllocator::AllocateMappedBufferWithNewBlock(const uint64_t aSize, const VkBufferUsageFlags aUsage, const VkMemoryPropertyFlags aProperty, MemoryPropertyCache& aCache)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = VkDrawConstants::SINGLEALLOCSIZE;
	bufferInfo.usage = aUsage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	AM_VkBuffer vkBuffer(bufferInfo);
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(VkDrawContext::device, vkBuffer.myBuffer, &memRequirements);
	uint32_t memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, aProperty);
	MemoryRequirements& req = aCache.myMemReqByBufferUsage[aUsage];
	req.myAlignment = memRequirements.alignment;
	req.myMemoryTypeIndex = memoryTypeIndex;

	std::vector<AM_BufferMemoryBlock>& bufferMemPool = myBufferMemoryPool[memoryTypeIndex];
	AM_BufferMemoryBlock& newBlock = bufferMemPool.emplace_back();
	newBlock.Init(vkBuffer, memoryTypeIndex, req.myAlignment);
	AM_Buffer* buffer = newBlock.Allocate(aSize + (newBlock.myAlignment - (aSize % newBlock.myAlignment)));

	vkMapMemory(VkDrawContext::device, newBlock.myMemory, 0, VkDrawConstants::SINGLEALLOCSIZE, 0, &newBlock.myMappedMemory);
	newBlock.myIsMapped = true;
	buffer->SetMappedMemory(newBlock.myMappedMemory);
	return buffer;
}

AM_Buffer* AM_NaiveMemoryAllocator::AllocateMappedBufferFast(const uint64_t aSize, const MemoryRequirements& aRequirement)
{
	std::vector<AM_BufferMemoryBlock>& bufferMemPool = myBufferMemoryPool[aRequirement.myMemoryTypeIndex];
	for (AM_BufferMemoryBlock& block : bufferMemPool)
	{
		if (block.myAlignment != aRequirement.myAlignment)
			continue;

		const uint64_t extraBytes = aSize % block.myAlignment;
		const uint64_t paddedSize = extraBytes ? aSize + block.myAlignment - extraBytes : aSize;
		if (block.myExtent + paddedSize > VkDrawConstants::SINGLEALLOCSIZE)
			continue;

		AM_Buffer* buffer = block.Allocate(paddedSize);
		if (!block.myIsMapped)
		{
			vkMapMemory(VkDrawContext::device, block.myMemory, 0, VkDrawConstants::SINGLEALLOCSIZE, 0, &block.myMappedMemory);
			block.myIsMapped = true;
		}
		buffer->SetMappedMemory(block.myMappedMemory);
		return buffer;
	}
	return nullptr;
}

AM_Buffer* AM_NaiveMemoryAllocator::AllocateMappedBufferSlow(const uint64_t aSize, const MemoryRequirements& aRequirement)
{
	std::vector<AM_BufferMemoryBlock>& bufferMemPool = myBufferMemoryPool[aRequirement.myMemoryTypeIndex];
	for (AM_BufferMemoryBlock& block : bufferMemPool)
	{
		if (block.myAlignment != aRequirement.myAlignment)
			continue;

		for (auto slotIter = block.myAllocationList.begin(); slotIter != block.myAllocationList.end(); ++slotIter)
		{
			const uint64_t extraBytes = aSize % block.myAlignment;
			const uint64_t paddedSize = extraBytes ? aSize + block.myAlignment - extraBytes : aSize;
			if (!(slotIter->IsEmpty() && slotIter->GetSize() >= paddedSize))
				continue;

			const uint64_t leftover = slotIter->GetSize() - paddedSize;
			if (leftover == 0)
				return &(*slotIter);

			block.myAllocationList.emplace(slotIter, block.myBuffer.myBuffer, slotIter->GetOffset(), leftover);
			if (!block.myIsMapped)
			{
				vkMapMemory(VkDrawContext::device, block.myMemory, 0, VkDrawConstants::SINGLEALLOCSIZE, 0, &block.myMappedMemory);
				block.myIsMapped = true;
			}
			slotIter->SetMappedMemory(block.myMappedMemory);
			slotIter->SetOffset(slotIter->GetOffset() + leftover);
			slotIter->SetSize(paddedSize);
			slotIter->SetIsEmpty(false);
			return &(*slotIter);
		}
	}

	return nullptr;
}

void AM_NaiveMemoryAllocator::CopyToMappedMemory(AM_Buffer& aBuffer, void* aSource, const size_t aSize)
{
	assert(aBuffer.GetMappedMemory() != nullptr && "Memory is not mapped!");
	assert(aSize <= aBuffer.GetSize() && "Trying to map more memory than allocated!");
	memcpy((void*)((char*)aBuffer.GetMappedMemory() + aBuffer.GetOffset()), aSource, aSize);
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
