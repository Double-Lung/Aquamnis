#pragma once
#include "VkDrawContext.h"

struct AM_StagingBuffer
{
	AM_StagingBuffer(const VkDeviceSize aBufferSize, const VkDrawContext& aContext);
	~AM_StagingBuffer();
	
	uint64_t myMemSize;
	VkBuffer myBuffer;
	VkDeviceMemory myMemory;
	const VkDrawContext& myContext;

private:
	AM_StagingBuffer() = default;
	// TODO: move to VkDrawContext
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};

