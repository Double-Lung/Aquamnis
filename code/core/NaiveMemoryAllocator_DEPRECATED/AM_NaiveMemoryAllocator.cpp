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
	const bool shouldMap = aProperty == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
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
		if (AM_Buffer* buffer = AllocateFast<AM_Buffer>(aSize, aUsage, *memReq, myBufferMemoryPool, shouldMap))
			return buffer;

		if (AM_Buffer* buffer = AllocateSlow<AM_Buffer>(aSize, aUsage, *memReq, myBufferMemoryPool, shouldMap))
			return buffer;
	}

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = AM_VkRenderCoreConstants::SINGLEALLOCSIZE;
	bufferInfo.usage = aUsage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	AM_VkBuffer vkBuffer(bufferInfo);
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(myVkContext.device, vkBuffer.myBuffer, &memRequirements);
	uint32_t memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, aProperty);
	MemoryRequirements& req = it->myMemReqByBufferUsage[aUsage];
	req.myAlignment = memRequirements.alignment;
	req.myMemoryTypeIndex = memoryTypeIndex;

	AM_Buffer* buffer = AllocateWithNewBlock<AM_Buffer>(aSize, aUsage, req, myBufferMemoryPool, shouldMap);
	assert(buffer != nullptr && "failed to allocate!");
	buffer->myBuffer = vkBuffer.myBuffer;
	vkBindBufferMemory(myVkContext.device, vkBuffer.myBuffer, buffer->GetMemoryHandle(), buffer->GetOffset());
	myBufferMemoryPool[memoryTypeIndex].back().myBuffer = std::move(vkBuffer);

	return buffer;
}

AM_Image* AM_NaiveMemoryAllocator::AllocateImage(const VkImageCreateInfo& aCreateInfo, const VkMemoryPropertyFlags aProperty)
{
	AM_VkImage image(aCreateInfo);
	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(myVkContext.device, image.GetImage(), &memReq);
	uint32_t memoryTypeIndex = FindMemoryTypeIndex(memReq.memoryTypeBits, aProperty);
	MemoryRequirements req{ memReq.alignment, memoryTypeIndex };

	AM_Image* imagePtr = AllocateFast<AM_Image>(memReq.size, aProperty, req, myImageMemoryPool);
	if (!imagePtr)
		imagePtr = AllocateSlow<AM_Image>(memReq.size, aProperty, req, myImageMemoryPool);
	if (!imagePtr)
		imagePtr = AllocateWithNewBlock<AM_Image>(memReq.size, aProperty, req, myImageMemoryPool);
	assert(imagePtr!=nullptr && "failed to allocate!");
	imagePtr->SetImage(std::move(image));
	vkBindImageMemory(myVkContext.device, imagePtr->GetImage(), imagePtr->GetMemoryHandle(), imagePtr->GetOffset());
	return imagePtr;
}

void AM_NaiveMemoryAllocator::CopyToMappedMemory(AM_Buffer& aBuffer, void* aSource, const size_t aSize)
{
	assert(aBuffer.GetMappedMemory() != nullptr && "Memory is not mapped!");
	assert(aSize <= aBuffer.GetSize() && "Trying to map more memory than allocated!");
	memcpy((void*)((char*)aBuffer.GetMappedMemory() + aBuffer.GetOffset()), aSource, aSize);
}
