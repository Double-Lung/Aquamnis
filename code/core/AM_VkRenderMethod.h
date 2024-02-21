#pragma once
#include "AM_VkContext.h"
#include "AM_VkPipeline.h"
#include "AM_PipelineUtils.h"
#include "AM_FrameRenderInfo.h"
#include "AM_VkDescriptorSetLayoutBuilder.h"
#include <unordered_map>

#define THIS(DERIVED) static_cast<DERIVED*>(this)

struct TempBuffer;
class AM_Entity;
class AM_Camera;

template <class ImpClass>
class AM_VkRenderMethod
{
public:
	AM_VkRenderMethod(
		AM_VkContext& aVkContext,
		const VkRenderPass aRenderPass,
		const std::string& aVertexShaderPath,
		const std::string& aFragmentShaderPath,
		uint32_t aBindingDescriptionCount,
		uint32_t anAttributeDescriptionCount,
		const VkVertexInputBindingDescription* aBindingDescription,
		const VkVertexInputAttributeDescription* anAttributeDescription,
		VkDescriptorSetLayout aGlobalLayout);

	AM_VkRenderMethod(const AM_VkRenderMethod&) = delete;
	AM_VkRenderMethod& operator=(const AM_VkRenderMethod&) = delete;

	void Render(AM_FrameRenderInfo& someInfo, std::vector<AM_Entity*> someEntities, const TempBuffer* aBuffer = nullptr);
	VkDescriptorSetLayout GetDescriptorSetLayout() { return myPipeline.GetDescriptorSetLayout(); }

protected:
	AM_VkContext& myVkContext;
	AM_VkPipeline myPipeline;

private:
	void CreatePipeline(
		const VkRenderPass aRenderPass,
		const std::string& aVertexShaderPath,
		const std::string& aFragmentShaderPath,
		uint32_t aBindingDescriptionCount,
		uint32_t anAttributeDescriptionCount,
		const VkVertexInputBindingDescription* aBindingDescription,
		const VkVertexInputAttributeDescription* anAttributeDescription,
		VkDescriptorSetLayout aGlobalLayout);
};

template <class ImpClass>
AM_VkRenderMethod<ImpClass>::AM_VkRenderMethod(AM_VkContext& aVkContext, const VkRenderPass aRenderPass, const std::string& aVertexShaderPath, const std::string& aFragmentShaderPath, uint32_t aBindingDescriptionCount, uint32_t anAttributeDescriptionCount, const VkVertexInputBindingDescription* aBindingDescription, const VkVertexInputAttributeDescription* anAttributeDescription, VkDescriptorSetLayout aGlobalLayout)
	: myVkContext(aVkContext)
	, myPipeline(aVkContext)
{
	CreatePipeline(
		aRenderPass,
		aVertexShaderPath,
		aFragmentShaderPath,
		aBindingDescriptionCount,
		anAttributeDescriptionCount,
		aBindingDescription,
		anAttributeDescription,
		aGlobalLayout);
}

template <class ImpClass>
void AM_VkRenderMethod<ImpClass>::Render(AM_FrameRenderInfo& someInfo, std::vector<AM_Entity*> someEntities, const TempBuffer* aBuffer /*= nullptr*/)
{
	THIS(ImpClass)->Render_Imp(someInfo, someEntities, aBuffer);
}

template <class ImpClass>
void AM_VkRenderMethod<ImpClass>::CreatePipeline(const VkRenderPass aRenderPass, const std::string& aVertexShaderPath, const std::string& aFragmentShaderPath, uint32_t aBindingDescriptionCount, uint32_t anAttributeDescriptionCount, const VkVertexInputBindingDescription* aBindingDescription, const VkVertexInputAttributeDescription* anAttributeDescription, VkDescriptorSetLayout aGlobalLayout)
{
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.renderPass = aRenderPass;

	AM_PipelineUtils::GraphicsInitializer graphicsInitializer{};
	AM_PipelineUtils::GetDefaultStates(graphicsInitializer);
	if (VK_SAMPLE_COUNT_1_BIT < myVkContext.globalMSAASamples)
		AM_PipelineUtils::EnableMultiSampleState(graphicsInitializer, myVkContext.globalMSAASamples);

	VkPipelineVertexInputStateCreateInfo& vertexInputInfo = graphicsInitializer.vertexInputState;
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = aBindingDescription ? aBindingDescriptionCount : 0;
	vertexInputInfo.pVertexBindingDescriptions = aBindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = anAttributeDescription ? anAttributeDescriptionCount : 0;
	vertexInputInfo.pVertexAttributeDescriptions = anAttributeDescription;

	AM_VkDescriptorSetLayoutBuilder builder;
	VkPushConstantRange pushConstantRange{};
	THIS(ImpClass)->CreatePipeline_Imp(builder, pushConstantRange, graphicsInitializer);

	VkPushConstantRange* rangePtr = pushConstantRange.size > 0 ? &pushConstantRange : nullptr;
	AM_PipelineUtils::FillPiplineCreateInfo(pipelineInfo, graphicsInitializer);
	myPipeline.CreatePipeline(aVertexShaderPath, aFragmentShaderPath, aGlobalLayout, builder, pipelineInfo, rangePtr);
}

#undef THIS
