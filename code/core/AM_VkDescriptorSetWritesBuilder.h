#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class AM_VkDescriptorSetWritesBuilder
{
public:
	AM_VkDescriptorSetWritesBuilder(VkDescriptorSetLayout aDescriptorSetLayout, VkDescriptorPool aDescriptorPool);

	void WriteBuffer(uint32_t aBinding, VkDescriptorBufferInfo* aBufferInfo, VkDescriptorType aType);
	void WriteImage(uint32_t aBinding, VkDescriptorImageInfo* anImageInfo, VkDescriptorType aType);
	std::vector<VkWriteDescriptorSet>& GetWrites() { return myWrites; }

private:
	VkWriteDescriptorSet& WriteDefault(uint32_t aBinding);

	VkDescriptorSetLayout myDescriptorSetLayout;
	VkDescriptorPool myDescriptorPool;
	std::vector<VkWriteDescriptorSet> myWrites;
};