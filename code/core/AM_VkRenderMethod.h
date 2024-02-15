#pragma once
#include "AM_VkContext.h"
#include "AM_VkPipeline.h"
#include "AM_PipelineUtils.h"
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
		const VkVertexInputAttributeDescription* anAttributeDescription);

	AM_VkRenderMethod(const AM_VkRenderMethod&) = delete;
	AM_VkRenderMethod& operator=(const AM_VkRenderMethod&) = delete;

	// #FIX_ME: refactor into a common interface
	void Render(VkCommandBuffer aCommandBuffer, VkDescriptorSet aDescriptorSet, const TempBuffer* aBuffer, const AM_Camera& aCamera);
	void Render(VkCommandBuffer aCommandBuffer, VkDescriptorSet aDescriptorSet, std::unordered_map<uint64_t, AM_Entity>& someEntites, const AM_Camera& aCamera);

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
		const VkVertexInputAttributeDescription* anAttributeDescription);
};


template <class ImpClass>
AM_VkRenderMethod<ImpClass>::AM_VkRenderMethod(AM_VkContext& aVkContext, const VkRenderPass aRenderPass, const std::string& aVertexShaderPath, const std::string& aFragmentShaderPath, uint32_t aBindingDescriptionCount, uint32_t anAttributeDescriptionCount, const VkVertexInputBindingDescription* aBindingDescription, const VkVertexInputAttributeDescription* anAttributeDescription)
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
		anAttributeDescription);
}

template <class ImpClass>
void AM_VkRenderMethod<ImpClass>::Render(VkCommandBuffer aCommandBuffer, VkDescriptorSet aDescriptorSet, std::unordered_map<uint64_t, AM_Entity>& someEntites, const AM_Camera& aCamera)
{
	THIS(ImpClass)->Render_Imp(aCommandBuffer, aDescriptorSet, someEntites, aCamera);
}

template <class ImpClass>
void AM_VkRenderMethod<ImpClass>::Render(VkCommandBuffer aCommandBuffer, VkDescriptorSet aDescriptorSet, const TempBuffer* aBuffer, const AM_Camera& aCamera)
{
	THIS(ImpClass)->Render_Imp(aCommandBuffer, aDescriptorSet, aBuffer, aCamera);
}

template <class ImpClass>
void AM_VkRenderMethod<ImpClass>::CreatePipeline(const VkRenderPass aRenderPass, const std::string& aVertexShaderPath, const std::string& aFragmentShaderPath, uint32_t aBindingDescriptionCount, uint32_t anAttributeDescriptionCount, const VkVertexInputBindingDescription* aBindingDescription, const VkVertexInputAttributeDescription* anAttributeDescription)
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
	myPipeline.CreatePipeline(aVertexShaderPath, aFragmentShaderPath, builder, pipelineInfo, rangePtr);
}
