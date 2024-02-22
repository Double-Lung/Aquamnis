#include "AM_VkDescriptorSetLayoutBuilder.h"

#include <cassert>

void AM_VkDescriptorSetLayoutBuilder::AddBinding(uint32_t aBinding, VkDescriptorType aDescriptorType, VkShaderStageFlags someShaderStageFlags, uint32_t aCount)
{
	assert(myBindings.count(aBinding) == 0 && "Binding already in use");
	VkDescriptorSetLayoutBinding& layoutBinding = myBindings[aBinding];
	layoutBinding.binding = aBinding;
	layoutBinding.descriptorType = aDescriptorType;
	layoutBinding.stageFlags = someShaderStageFlags;
	layoutBinding.descriptorCount = aCount;
}

void AM_VkDescriptorSetLayoutBuilder::GetBindings(std::vector<VkDescriptorSetLayoutBinding>& outBindings)
{
	outBindings.clear();
	outBindings.reserve(myBindings.size());
	for (auto& entry : myBindings)
		outBindings.push_back(entry.second);
}
