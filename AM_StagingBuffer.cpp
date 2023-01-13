#include "AM_StagingBuffer.h"

AM_StagingBuffer::AM_StagingBuffer(const VkDeviceSize aBufferSize, const VkDrawContext& aContext)
	: myMemSize(0)
	, myBuffer(nullptr)
	, myMemory(nullptr)
	, myContext(aContext)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = aBufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(VkDrawContext::device, &bufferInfo, nullptr, &myBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create staging buffer!");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(VkDrawContext::device, myBuffer, &memRequirements);
	myMemSize = memRequirements.size;

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = myMemSize;
	// TODO: cache
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(VkDrawContext::device, &allocInfo, nullptr, &myMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate buffer memory!");

	vkBindBufferMemory(VkDrawContext::device, myBuffer, myMemory, 0);
}

AM_StagingBuffer::~AM_StagingBuffer()
{
	vkDestroyBuffer(VkDrawContext::device, myBuffer, nullptr);
	vkFreeMemory(VkDrawContext::device, myMemory, nullptr);
}

uint32_t AM_StagingBuffer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	for (uint32_t i = 0; i < myContext.memoryProperties.memoryTypeCount; ++i)
		if ((typeFilter & (1 << i)) && (myContext.memoryProperties.memoryTypes[i].propertyFlags ^ properties) == 0)
			return i;

	throw std::runtime_error("failed to find suitable memory type!");
}
