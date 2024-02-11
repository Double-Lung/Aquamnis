#include "AM_VkDescriptorSetLayout.h"
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