#include "AM_VkDescriptorPool.h"

void AM_VkDescriptorPool::CreatePool(uint32_t aMaxSets, VkDescriptorPoolCreateFlags somePoolFlags, const std::vector<VkDescriptorPoolSize>& somePoolSizes)
{
	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(somePoolSizes.size());
	descriptorPoolInfo.pPoolSizes = somePoolSizes.data();
	descriptorPoolInfo.maxSets = aMaxSets;
	descriptorPoolInfo.flags = somePoolFlags;

	if (vkCreateDescriptorPool(myVkContext.device, &descriptorPoolInfo, nullptr, &myPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool!");
}

void AM_VkDescriptorPool::AllocateDescriptorSet(const VkDescriptorSetLayout aDescriptorSetLayout, VkDescriptorSet& aDescriptorSet)
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = myPool;
	allocInfo.pSetLayouts = &aDescriptorSetLayout;
	allocInfo.descriptorSetCount = 1;

	if (vkAllocateDescriptorSets(myVkContext.device, &allocInfo, &aDescriptorSet) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set from pool!");
}

void AM_VkDescriptorPool::AllocateDescriptorSets(std::vector<VkDescriptorSetLayout>& someDescriptorSetLayouts, std::vector<VkDescriptorSet>& someDescriptorSets)
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = myPool;
	allocInfo.pSetLayouts = someDescriptorSetLayouts.data();
	allocInfo.descriptorSetCount = static_cast<uint32_t>(someDescriptorSets.size());

	if (vkAllocateDescriptorSets(myVkContext.device, &allocInfo, someDescriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set from pool!");
}