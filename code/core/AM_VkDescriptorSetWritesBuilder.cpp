#include "AM_VkDescriptorSetWritesBuilder.h"
#include <cassert>

AM_VkDescriptorSetWritesBuilder::AM_VkDescriptorSetWritesBuilder(VkDescriptorPool aDescriptorPool) 
	: myDescriptorPool{ aDescriptorPool }
{
}

VkWriteDescriptorSet& AM_VkDescriptorSetWritesBuilder::WriteDefault(uint32_t aBinding)
{
	VkWriteDescriptorSet& write = myWrites.emplace_back();
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = aBinding;
	write.descriptorCount = 1;
	return write;
}

void AM_VkDescriptorSetWritesBuilder::WriteBuffer(uint32_t aBinding, VkDescriptorBufferInfo* aBufferInfo, VkDescriptorType aType)
{
	VkWriteDescriptorSet& write = WriteDefault(aBinding);
	write.pBufferInfo = aBufferInfo;
	write.descriptorType = aType;
}

void AM_VkDescriptorSetWritesBuilder::WriteImage(uint32_t aBinding, VkDescriptorImageInfo* anImageInfo, VkDescriptorType aType)
{
	VkWriteDescriptorSet& write = WriteDefault(aBinding);
	write.pImageInfo = anImageInfo;
	write.descriptorType = aType;
}
