#pragma once
#include "AM_VkContext.h"
#include <unordered_map>

class AM_VkDescriptorSetLayout
{
	friend class AM_VkDescriptorSetWriter;

public:
	AM_VkDescriptorSetLayout(AM_VkContext& aVkContext)
		: myVkContext{ aVkContext }
		, myLayout(VK_NULL_HANDLE)
	{
	}
	~AM_VkDescriptorSetLayout() { vkDestroyDescriptorSetLayout(myVkContext.device, myLayout, nullptr); }
	const VkDescriptorSetLayout& GetDescriptorSetLayout() const { return myLayout; }
	VkDescriptorSetLayout& GetDescriptorSetLayout() { return myLayout; }

	void CreateLayout();
	void AddBinding(uint32_t aBinding, VkDescriptorType aDescriptorType, VkShaderStageFlags someShaderStageFlags, uint32_t aCount);

private:
	AM_VkDescriptorSetLayout(const AM_VkDescriptorSetLayout&) = delete;
	AM_VkDescriptorSetLayout& operator=(const AM_VkDescriptorSetLayout&) = delete;

	AM_VkContext& myVkContext;
	VkDescriptorSetLayout myLayout;
	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> myBindings;
};