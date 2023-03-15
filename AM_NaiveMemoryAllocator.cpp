#include "AM_NaiveMemoryAllocator.h"
#include <assert.h>

void AM_NaiveMemoryAllocator::Init(const VkPhysicalDeviceMemoryProperties& aPhysicalMemoryProperties)
{
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

void AM_NaiveMemoryAllocator::AllocateMemory(VkDeviceMemory& outMemoryPtr, const uint32_t aMemoryTypeIndex)
{
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = AM_VkRenderCoreConstants::SINGLEALLOCSIZE;
	allocInfo.memoryTypeIndex = aMemoryTypeIndex;
	if (vkAllocateMemory(AM_VkContext::device, &allocInfo, nullptr, &outMemoryPtr) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate memory of type ??? !");
}

uint32_t AM_NaiveMemoryAllocator::FindMemoryTypeIndex(const uint32_t someMemoryTypeBits, const VkMemoryPropertyFlags someProperties) const
{
	const VkMemoryType* memoryTypes = myPhysicalMemoryProperties.memoryTypes;
	for (uint32_t i = 0; i < myPhysicalMemoryProperties.memoryTypeCount; ++i)
		if ((someMemoryTypeBits & (1 << i)) && (memoryTypes[i].propertyFlags == someProperties))
			return i;

	throw std::runtime_error("failed to find suitable memory type!");
}

uint64_t AM_NaiveMemoryAllocator::GetPaddedSize(const uint64_t aSize, const uint64_t& anAlignmentSize) const
{
	const uint64_t extraBytes = aSize % anAlignmentSize;
	return extraBytes ? aSize + anAlignmentSize - extraBytes : aSize;
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

AM_Image* AM_NaiveMemoryAllocator::AllocateImage(const VkImageCreateInfo& aCreateInfo, const VkMemoryPropertyFlags aProperty)
{
	AM_VkImage image(aCreateInfo);
	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(AM_VkContext::device, image.myImage, &memReq);
	uint32_t memoryTypeIndex = FindMemoryTypeIndex(memReq.memoryTypeBits, aProperty);
	
	if (AM_Image* imagePtr = AllocateImageFast(image, memReq, memoryTypeIndex))
		return imagePtr;
	if (AM_Image* imagePtr = AllocateImageSlow(image, memReq, memoryTypeIndex))
		return imagePtr;

	return AllocateImageWithNewBlock(image, memReq, memoryTypeIndex);
}

AM_Buffer* AM_NaiveMemoryAllocator::AllocateBufferWithNewBlock(const uint64_t aSize, const VkBufferUsageFlags aUsage, const VkMemoryPropertyFlags aProperty, MemoryPropertyCache& aCache)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = AM_VkRenderCoreConstants::SINGLEALLOCSIZE;
	bufferInfo.usage = aUsage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	AM_VkBuffer vkBuffer(bufferInfo);
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(AM_VkContext::device, vkBuffer.myBuffer, &memRequirements);
	uint32_t memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, aProperty);
	MemoryRequirements& req = aCache.myMemReqByBufferUsage[aUsage];
	req.myAlignment = memRequirements.alignment;
	req.myMemoryTypeIndex = memoryTypeIndex;

	std::vector<AM_BufferMemoryBlock>& bufferMemPool = myBufferMemoryPool[memoryTypeIndex];
	AM_BufferMemoryBlock& newBlock = bufferMemPool.emplace_back();
	newBlock.Init(vkBuffer, memoryTypeIndex, req.myAlignment);

	return newBlock.Allocate(GetPaddedSize(aSize, req.myAlignment));
}

AM_Buffer* AM_NaiveMemoryAllocator::AllocateBufferFast(const uint64_t aSize, const MemoryRequirements& aRequirement)
{
	std::vector<AM_BufferMemoryBlock>& bufferMemPool = myBufferMemoryPool[aRequirement.myMemoryTypeIndex];
	for (AM_BufferMemoryBlock& block : bufferMemPool)
	{
		if (block.myAlignment != aRequirement.myAlignment)
			continue;

		const uint64_t paddedSize = GetPaddedSize(aSize, block.myAlignment); 
		if (block.myExtent + paddedSize > AM_VkRenderCoreConstants::SINGLEALLOCSIZE)
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
			const uint64_t paddedSize = GetPaddedSize(aSize, block.myAlignment); 
			if (!(slotIter->IsEmpty() && slotIter->GetSize() >= paddedSize))
				continue;

			const uint64_t leftover = slotIter->GetSize() - paddedSize;
			if (leftover == 0)
			{
				slotIter->SetIsEmpty(false);
				return &(*slotIter);
			}

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
	bufferInfo.size = AM_VkRenderCoreConstants::SINGLEALLOCSIZE;
	bufferInfo.usage = aUsage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	AM_VkBuffer vkBuffer(bufferInfo);
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(AM_VkContext::device, vkBuffer.myBuffer, &memRequirements);
	uint32_t memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, aProperty);
	MemoryRequirements& req = aCache.myMemReqByBufferUsage[aUsage];
	req.myAlignment = memRequirements.alignment;
	req.myMemoryTypeIndex = memoryTypeIndex;

	std::vector<AM_BufferMemoryBlock>& bufferMemPool = myBufferMemoryPool[memoryTypeIndex];
	AM_BufferMemoryBlock& newBlock = bufferMemPool.emplace_back();
	newBlock.Init(vkBuffer, memoryTypeIndex, req.myAlignment);
	AM_Buffer* buffer = newBlock.Allocate(GetPaddedSize(aSize, req.myAlignment));

	vkMapMemory(AM_VkContext::device, newBlock.myMemory, 0, AM_VkRenderCoreConstants::SINGLEALLOCSIZE, 0, &newBlock.myMappedMemory);
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

		const uint64_t paddedSize = GetPaddedSize(aSize, block.myAlignment);
		if (block.myExtent + paddedSize > AM_VkRenderCoreConstants::SINGLEALLOCSIZE)
			continue;

		AM_Buffer* buffer = block.Allocate(paddedSize);
		if (!block.myIsMapped)
		{
			vkMapMemory(AM_VkContext::device, block.myMemory, 0, AM_VkRenderCoreConstants::SINGLEALLOCSIZE, 0, &block.myMappedMemory);
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
			const uint64_t paddedSize = GetPaddedSize(aSize, block.myAlignment);
			if (!(slotIter->IsEmpty() && slotIter->GetSize() >= paddedSize))
				continue;

			const uint64_t leftover = slotIter->GetSize() - paddedSize;
			if (leftover == 0)
			{
				slotIter->SetIsEmpty(false);
				return &(*slotIter);
			}

			block.myAllocationList.emplace(slotIter, block.myBuffer.myBuffer, slotIter->GetOffset(), leftover);
			if (!block.myIsMapped)
			{
				vkMapMemory(AM_VkContext::device, block.myMemory, 0, AM_VkRenderCoreConstants::SINGLEALLOCSIZE, 0, &block.myMappedMemory);
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

AM_Image* AM_NaiveMemoryAllocator::AllocateImageFast(AM_VkImage& outImage, const VkMemoryRequirements& aRequirement, const uint32_t aMemoryTypeIndex)
{
	std::vector<AM_ImageMemoryBlock>& imageMemoryPool = myImageMemoryPool[aMemoryTypeIndex];
	for (AM_ImageMemoryBlock& block : imageMemoryPool)
	{
		if (block.myAlignment != aRequirement.alignment)
			continue;

		const uint64_t paddedSize = GetPaddedSize(aRequirement.size, block.myAlignment); aRequirement.size;
		if (block.myExtent + paddedSize > AM_VkRenderCoreConstants::SINGLEALLOCSIZE)
			continue;

		return block.Allocate(outImage, paddedSize);
	}
	return nullptr;
}

AM_Image* AM_NaiveMemoryAllocator::AllocateImageSlow(AM_VkImage& outImage, const VkMemoryRequirements& aRequirement, const uint32_t aMemoryTypeIndex)
{
	std::vector<AM_ImageMemoryBlock>& imageMemoryPool = myImageMemoryPool[aMemoryTypeIndex];
	for (AM_ImageMemoryBlock& block : imageMemoryPool)
	{
		if (block.myAlignment != aRequirement.alignment)
			continue;

		for (auto slotIter = block.myAllocationList.begin(); slotIter != block.myAllocationList.end(); ++slotIter)
		{
			const uint64_t paddedSize = GetPaddedSize(aRequirement.size, block.myAlignment); 
			if (!(slotIter->IsEmpty() && slotIter->GetSize() >= paddedSize))
				continue;

			const uint64_t leftover = slotIter->GetSize() - paddedSize;
			if (leftover == 0)
			{
				slotIter->SetIsEmpty(false);
				return &(*slotIter);
			}

			block.myAllocationList.emplace(slotIter, slotIter->GetOffset(), leftover);
			slotIter->SetImage(std::move(outImage));
			slotIter->SetOffset(slotIter->GetOffset() + leftover);
			slotIter->SetSize(paddedSize);
			slotIter->SetIsEmpty(false);
			return &(*slotIter);
		}
	}

	return nullptr;
}

AM_Image* AM_NaiveMemoryAllocator::AllocateImageWithNewBlock(AM_VkImage& outImage, const VkMemoryRequirements& aRequirement, const uint32_t aMemoryTypeIndex)
{
	std::vector<AM_ImageMemoryBlock>& imageMemoryPool = myImageMemoryPool[aMemoryTypeIndex];
	AM_ImageMemoryBlock& newBlock = imageMemoryPool.emplace_back();
	newBlock.Init(aMemoryTypeIndex, aRequirement.alignment);
	return newBlock.Allocate(outImage, GetPaddedSize(aRequirement.size, aRequirement.alignment));
}

void AM_NaiveMemoryAllocator::CopyToMappedMemory(AM_Buffer& aBuffer, void* aSource, const size_t aSize)
{
	assert(aBuffer.GetMappedMemory() != nullptr && "Memory is not mapped!");
	assert(aSize <= aBuffer.GetSize() && "Trying to map more memory than allocated!");
	memcpy((void*)((char*)aBuffer.GetMappedMemory() + aBuffer.GetOffset()), aSource, aSize);
}
