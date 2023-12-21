#include "AM_VkDescriptorUtils.h"
#include <cassert>

void AM_VkDescriptorSetLayout::CreateLayout()
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	bindings.reserve(myBindings.size());
	for (auto& entry : myBindings)
		bindings.push_back(entry.second);

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	descriptorSetLayoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(myVkContext.device, &descriptorSetLayoutInfo, nullptr, &myLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set layout!");
}

void AM_VkDescriptorSetLayout::AddBinding(uint32_t aBinding, VkDescriptorType aDescriptorType, VkShaderStageFlags someShaderStageFlags, uint32_t aCount)
{
	assert(myBindings.count(aBinding) == 0 && "Binding already in use");
	VkDescriptorSetLayoutBinding& layoutBinding = myBindings[aBinding];
	layoutBinding.binding = aBinding;
	layoutBinding.descriptorType = aDescriptorType;
	layoutBinding.stageFlags = someShaderStageFlags;
	layoutBinding.descriptorCount = aCount;
}

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

