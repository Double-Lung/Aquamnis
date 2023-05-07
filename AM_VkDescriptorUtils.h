#pragma once

#include "AM_VkContext.h"
#include <memory>
#include <unordered_map>
#include <vector>

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

class AM_VkDescriptorPool
{
	friend class AM_VkDescriptorSetWriter;

public:
	AM_VkDescriptorPool(AM_VkContext& aVkContext)
		: myVkContext{aVkContext}
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

class AM_VkDescriptorSetWriter
{
public:
	AM_VkDescriptorSetWriter(AM_VkDescriptorSetLayout& aDescriptorSetLayout, AM_VkDescriptorPool& aDescriptorPool)
		: myDescriptorSetLayout{aDescriptorSetLayout}
		, myPool{aDescriptorPool}
	{
	}
	
	void WriteBuffer(uint32_t aBinding, VkDescriptorBufferInfo* aBufferInfo);
	void WriteImage(uint32_t aBinding, VkDescriptorImageInfo* anImageInfo);

	void Build(VkDescriptorSet& aDescriptorSet);
	void Update(VkDescriptorSet& aDescriptorSet);

private:
	VkWriteDescriptorSet& WriteDefault(uint32_t aBinding);

	AM_VkDescriptorSetLayout& myDescriptorSetLayout;
	AM_VkDescriptorPool& myPool;
	std::vector<VkWriteDescriptorSet> myWrites;
};