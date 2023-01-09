#pragma once

#include "VkDrawContext.h"

struct AM_StagingBuffer
{
	AM_StagingBuffer(const VkDeviceSize aBufferSize, const VkDrawContext& aContext);
	~AM_StagingBuffer();
	
	VkBuffer myBuffer;
	VkDeviceMemory myMemory;
	const VkDrawContext& myContext;

private:
	AM_StagingBuffer() = default;
	void Allocate(const VkDeviceSize anAllocSize, const uint32_t aMemoryTypeIndex);
	// TODO: move to VkDrawContext
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};

