#pragma once
#include "AM_VkContext.h"

class AM_VkDescriptorPool
{
	friend class AM_VkDescriptorSetWriter;

public:
	AM_VkDescriptorPool(AM_VkContext& aVkContext)
		: myVkContext{ aVkContext }
		, myPool(VK_NULL_HANDLE)
	{
	}

	~AM_VkDescriptorPool() { vkDestroyDescriptorPool(myVkContext.device, myPool, nullptr); }

	void CreatePool(uint32_t aMaxSets, VkDescriptorPoolCreateFlags somePoolFlags, const std::vector<VkDescriptorPoolSize>& somePoolSizes);
	const VkDescriptorPool& GetPool() const { return myPool; }
	VkDescriptorPool& GetPool() { return myPool; }

	void AllocateDescriptorSet(const VkDescriptorSetLayout aDescriptorSetLayout, VkDescriptorSet& aDescriptorSet);
	void AllocateDescriptorSets(std::vector<VkDescriptorSetLayout>& someDescriptorSetLayouts, std::vector<VkDescriptorSet>& someDescriptorSets);
	void FreeDescriptorSets(std::vector<VkDescriptorSet>& someDescriptorSets) const
	{
		vkFreeDescriptorSets(
			myVkContext.device,
			myPool,
			static_cast<uint32_t>(someDescriptorSets.size()),
			someDescriptorSets.data());
	}
	void ResetPool() { vkResetDescriptorPool(myVkContext.device, myPool, 0); }

private:
	AM_VkDescriptorPool(const AM_VkDescriptorPool&) = delete;
	AM_VkDescriptorPool& operator=(const AM_VkDescriptorPool&) = delete;

	AM_VkContext& myVkContext;
	VkDescriptorPool myPool;
};