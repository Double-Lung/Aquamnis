#include "AM_SimpleGPUParticleSystem.h"
#include <fstream>

AM_SimpleGPUParticleSystem::AM_SimpleGPUParticleSystem(AM_VkContext& aVkContext, VkRenderPass aRenderPass)
	: myVkContext{ aVkContext }
	, myComputePipeline{}
	, myPipelineLayout{}
	, myDescriptorSetLayout{ aVkContext }
{
	CreateDescriptorSetLayout();
	CreateComputePipeline(aRenderPass);

	myMaxComputeWorkGroupCount = myVkContext.deviceProperties.limits.maxComputeWorkGroupCount;
	myMaxComputeWorkGroupInvocations = myVkContext.deviceProperties.limits.maxComputeWorkGroupInvocations;
	myMaxComputeWorkGroupSize = myVkContext.deviceProperties.limits.maxComputeWorkGroupSize;
}

void AM_SimpleGPUParticleSystem::Render(VkCommandBuffer /*aCommandBuffer*/, VkDescriptorSet& /*aDescriptorSet*/)
{
	
}

void AM_SimpleGPUParticleSystem::DispatchWork(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet)
{
	vkCmdBindPipeline(aCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, myComputePipeline.myPipeline);
	vkCmdBindDescriptorSets(aCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, myPipelineLayout.myLayout, 0, 1, &aDescriptorSet, 0, nullptr);

	static constexpr int PARTICLE_COUNT = 100;
	vkCmdDispatch(aCommandBuffer, (PARTICLE_COUNT - 1) / 256 + 1, 1, 1);

	if (vkEndCommandBuffer(aCommandBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to record compute command buffer!");
}

void AM_SimpleGPUParticleSystem::CreateComputePipeline(VkRenderPass aRenderPass)
{
	auto computeShaderCode = ReadFile("../data/shader_bytecode/particle.comp.spv");
#ifdef _DEBUG
	std::cout << "compute shader file size: " << computeShaderCode.size() << '\n';
#endif
	VkShaderModule computeShaderModule = CreateShaderModule(computeShaderCode);

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = computeShaderModule;
	computeShaderStageInfo.pName = "main";

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &myDescriptorSetLayout.GetDescriptorSetLayout();

	myPipelineLayout.CreateLayout(pipelineLayoutInfo);

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = myPipelineLayout.myLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	myComputePipeline.CreatePipeline(pipelineInfo);

	vkDestroyShaderModule(AM_VkContext::device, computeShaderModule, nullptr);
}

std::vector<char> AM_SimpleGPUParticleSystem::ReadFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("failed to open file!");

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

VkShaderModule AM_SimpleGPUParticleSystem::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(AM_VkContext::device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module!");

	return shaderModule;
}

void AM_SimpleGPUParticleSystem::CreateDescriptorSetLayout()
{
	myDescriptorSetLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	myDescriptorSetLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	myDescriptorSetLayout.AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	myDescriptorSetLayout.CreateLayout();
}
