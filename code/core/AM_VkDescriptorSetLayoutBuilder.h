#pragma once

#include "AM_VkContext.h"
#include <unordered_map>

class AM_VkDescriptorSetLayoutBuilder
{
public:
	void AddBinding(uint32_t aBinding, VkDescriptorType aDescriptorType, VkShaderStageFlags someShaderStageFlags, uint32_t aCount);
	void GetBindings(std::vector<VkDescriptorSetLayoutBinding>& outBindings);

private:
	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> myBindings;
};