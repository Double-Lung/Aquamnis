#pragma once
#include "AM_VkContext.h"

class AM_VkDescriptorSetLayoutBuilder;
class AM_VkPipeline
{
public:
	explicit AM_VkPipeline(AM_VkContext& aVkContext);
	~AM_VkPipeline();
	AM_VkPipeline(const AM_VkPipeline&) = delete;
	AM_VkPipeline& operator=(const AM_VkPipeline&) = delete;

	void CreatePipeline(
		const std::string& aVertShaderPath, 
		const std::string& aFragShaderPath, 
		AM_VkDescriptorSetLayoutBuilder& aBuilder, 
		VkGraphicsPipelineCreateInfo& aPipelineCreateInfo,
		const VkPushConstantRange* aPushConstantRange = nullptr);

	void CreatePipeline(
		const std::string& aCompShaderPath, 
		AM_VkDescriptorSetLayoutBuilder& aBuilder, 
		VkComputePipelineCreateInfo& aPipelineCreateInfo,
		const VkPushConstantRange* aPushConstantRange = nullptr);

	void BindGraphics(VkCommandBuffer aCommandBuffer) { vkCmdBindPipeline(aCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myPipeline); }
	void BindCompute(VkCommandBuffer aCommandBuffer) { vkCmdBindPipeline(aCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, myPipeline); }

	VkDescriptorSetLayout GetDescriptorSetLayout() { return myDescriptorSetLayout; }
	VkPipelineLayout GetPipelineLayout() { return myPipelineLayout; }

private:
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	void CreateDescriptorSetLayout(AM_VkDescriptorSetLayoutBuilder& aBuilder);
	void CreatePipelineLayout(const VkPushConstantRange* aPushConstantRange = nullptr);

	AM_VkContext& myVkContext;
	VkDescriptorSetLayout myDescriptorSetLayout;
	VkPipelineLayout myPipelineLayout;
	VkPipeline myPipeline;
};