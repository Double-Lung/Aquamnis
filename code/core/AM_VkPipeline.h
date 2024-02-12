#pragma once
#include "AM_VkContext.h"

class AM_VkPipeline
{
public:
	explicit AM_VkPipeline(AM_VkContext& aVkContext);
	~AM_VkPipeline() { vkDestroyPipeline(myVkContext.device, myPipeline, nullptr); }
	AM_VkPipeline(const AM_VkPipeline&) = delete;
	AM_VkPipeline& operator=(const AM_VkPipeline&) = delete;

	void CreatePipeline(const std::string& aVertShaderPath, const std::string& aFragShaderPath, VkGraphicsPipelineCreateInfo& aPipelineCreateInfo);
	void CreatePipeline(const std::string& aCompShaderPath, VkComputePipelineCreateInfo& aPipelineCreateInfo);
	void BindGraphics(VkCommandBuffer aCommandBuffer) { vkCmdBindPipeline(aCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myPipeline); }
	void BindCompute(VkCommandBuffer aCommandBuffer) { vkCmdBindPipeline(aCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, myPipeline); }

private:
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	
	AM_VkContext& myVkContext;
	VkPipeline myPipeline;
};