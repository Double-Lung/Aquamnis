#include "AM_VkDescriptorSetWriter.h"
#include <cassert>

VkWriteDescriptorSet& AM_VkDescriptorSetWriter::WriteDefault(uint32_t aBinding)
{
	assert(myDescriptorSetLayout.myBindings.count(aBinding) == 1 && "Layout does not contain specified binding");
	VkDescriptorSetLayoutBinding& bindingDescription = myDescriptorSetLayout.myBindings[aBinding];
	assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

	VkWriteDescriptorSet& write = myWrites.emplace_back();
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = bindingDescription.descriptorType;
	write.dstBinding = aBinding;
	write.descriptorCount = 1;
	return write;
}

void AM_VkDescriptorSetWriter::WriteBuffer(uint32_t aBinding, VkDescriptorBufferInfo* aBufferInfo)
{
	VkWriteDescriptorSet& write = WriteDefault(aBinding);
	write.pBufferInfo = aBufferInfo;
}

void AM_VkDescriptorSetWriter::WriteImage(uint32_t aBinding, VkDescriptorImageInfo* anImageInfo)
{
	VkWriteDescriptorSet& write = WriteDefault(aBinding);
	write.pImageInfo = anImageInfo;
}

void AM_VkDescriptorSetWriter::Build(VkDescriptorSet& aDescriptorSet)
{
	myPool.AllocateDescriptorSet(myDescriptorSetLayout.GetDescriptorSetLayout(), aDescriptorSet);
	Update(aDescriptorSet);
}

void AM_VkDescriptorSetWriter::Update(VkDescriptorSet& aDescriptorSet)
{
	for (VkWriteDescriptorSet& write : myWrites)
	{
		write.dstSet = aDescriptorSet;
	}
	vkUpdateDescriptorSets(myPool.myVkContext.device, static_cast<uint32_t>(myWrites.size()), myWrites.data(), 0, nullptr);
}