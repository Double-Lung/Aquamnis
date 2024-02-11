#pragma once
#include "AM_VkDescriptorPool.h"
#include "AM_VkDescriptorSetLayout.h"

class AM_VkDescriptorSetWriter
{
public:
	AM_VkDescriptorSetWriter(AM_VkDescriptorSetLayout& aDescriptorSetLayout, AM_VkDescriptorPool& aDescriptorPool)
		: myDescriptorSetLayout{ aDescriptorSetLayout }
		, myPool{ aDescriptorPool }
	{
	}

	void WriteBuffer(uint32_t aBinding, VkDescriptorBufferInfo* aBufferInfo);
	void WriteImage(uint32_t aBinding, VkDescriptorImageInfo* anImageInfo);

	void Build(VkDescriptorSet& aDescriptorSet);
	void Update(VkDescriptorSet& aDescriptorSet);

private:
	VkWriteDescriptorSet& WriteDefault(uint32_t aBinding);

	AM_VkDescriptorSetLayout& myDescriptorSetLayout;
	AM_VkDescriptorPool& myPool;
	std::vector<VkWriteDescriptorSet> myWrites;
};