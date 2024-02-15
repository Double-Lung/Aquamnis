#include "AM_VkPipeline.h"
#include "AM_RenderUtils.h"
#include "AM_VkDescriptorSetLayoutBuilder.h"
#include <array>
#include <cassert>

AM_VkPipeline::AM_VkPipeline(AM_VkContext& aVkContext)
	: myVkContext(aVkContext)
	, myDescriptorSetLayout(nullptr)
	, myPipelineLayout(nullptr)
	, myPipeline(nullptr)
{
}

AM_VkPipeline::~AM_VkPipeline()
{
	vkDestroyPipeline(myVkContext.device, myPipeline, nullptr);
	vkDestroyPipelineLayout(myVkContext.device, myPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(myVkContext.device, myDescriptorSetLayout, nullptr);
}

void AM_VkPipeline::CreatePipeline(const std::string& aVertShaderPath, const std::string& aFragShaderPath, AM_VkDescriptorSetLayoutBuilder& aBuilder, VkGraphicsPipelineCreateInfo& aPipelineCreateInfo, const VkPushConstantRange* aPushConstantRange)
{
	assert(myPipeline == nullptr && "Trying to create pipeline again!");

	CreateDescriptorSetLayout(aBuilder);
	CreatePipelineLayout(aPushConstantRange);

	std::vector<char> vertCode;
	std::vector<char> fragCode;
	AM_RenderUtils::ReadFile(vertCode, aVertShaderPath);
	AM_RenderUtils::ReadFile(fragCode, aFragShaderPath);

#ifdef _DEBUG
	std::cout << "vert file size: " << vertCode.size() << '\n';
	std::cout << "frag file size: " << fragCode.size() << '\n';
#endif

	VkShaderModule vertModule = CreateShaderModule(vertCode);
	VkShaderModule fragModule = CreateShaderModule(fragCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

	vertShaderStageInfo.module = vertModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragModule;
	fragShaderStageInfo.pName = "main";

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };
	aPipelineCreateInfo.pStages = shaderStages.data();
	aPipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());

	aPipelineCreateInfo.layout = myPipelineLayout;

	if (vkCreateGraphicsPipelines(myVkContext.device, VK_NULL_HANDLE, 1, &aPipelineCreateInfo, nullptr, &myPipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline!");

	vkDestroyShaderModule(myVkContext.device, fragModule, nullptr);
	vkDestroyShaderModule(myVkContext.device, vertModule, nullptr);
}

void AM_VkPipeline::CreatePipeline(const std::string& aCompShaderPath, AM_VkDescriptorSetLayoutBuilder& aBuilder, VkComputePipelineCreateInfo& aPipelineCreateInfo, const VkPushConstantRange* aPushConstantRange)
{
	assert(myPipeline == nullptr && "Trying to create pipeline again!");

	CreateDescriptorSetLayout(aBuilder);
	CreatePipelineLayout(aPushConstantRange);

	std::vector<char> compCode;
	AM_RenderUtils::ReadFile(compCode, aCompShaderPath);

#ifdef _DEBUG
	std::cout << "comp file size: " << compCode.size() << '\n';
#endif

	VkShaderModule compModule = CreateShaderModule(compCode);
	aPipelineCreateInfo.stage.module = compModule;
	aPipelineCreateInfo.layout = myPipelineLayout;

	if (vkCreateComputePipelines(myVkContext.device, VK_NULL_HANDLE, 1, &aPipelineCreateInfo, nullptr, &myPipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create compute pipeline!");

	vkDestroyShaderModule(myVkContext.device, compModule, nullptr);
}

VkShaderModule AM_VkPipeline::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(myVkContext.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module!");

	return shaderModule;
}

void AM_VkPipeline::CreateDescriptorSetLayout(AM_VkDescriptorSetLayoutBuilder& aBuilder)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	aBuilder.GetBindings(bindings);
	myDescriptorSetLayout = myVkContext.CreateDescriptorSetLayout(bindings);
}

void AM_VkPipeline::CreatePipelineLayout(const VkPushConstantRange* aPushConstantRange)
{
	assert(myDescriptorSetLayout && "Invalid descriptor set layout!");

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &myDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = aPushConstantRange ? 1 : 0;
	pipelineLayoutInfo.pPushConstantRanges = aPushConstantRange;

	myPipelineLayout = myVkContext.CreatePipelineLayout(pipelineLayoutInfo);
}
