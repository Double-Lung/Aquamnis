#include "AM_PointLightRenderSystem.h"
#include "AM_Entity.h"
#include "AM_Camera.h"
#include "AM_SimpleTimer.h"
#include "AM_VkDescriptorSetLayoutBuilder.h"
#include "AM_RenderUtils.h"
#include "AM_PipelineUtils.h"
#include <array>
#include <algorithm>

AM_PointLightRenderSystem::AM_PointLightRenderSystem(AM_VkContext& aVkContext, VkRenderPass aRenderPass)
	: myVkContext{ aVkContext }
	, myGraphicsPipeline{aVkContext}
	, myPipelineLayout{nullptr}
{
	AM_VkDescriptorSetLayoutBuilder builder;
	builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1);
	builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

	std::vector<VkDescriptorSetLayoutBinding> bindings;
	builder.GetBindings(bindings);
	myDescriptorSetLayout = myVkContext.CreateDescriptorSetLayout(bindings);

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PointLightPushConstants);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &myDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional

	myPipelineLayout = myVkContext.CreatePipelineLayout(pipelineLayoutInfo);

	CreateGraphicsPipeline(aRenderPass);
}

void AM_PointLightRenderSystem::Render(VkCommandBuffer aCommandBuffer, VkDescriptorSet& aDescriptorSet, std::unordered_map<uint64_t, AM_Entity>& someEntites, const AM_Camera& aCamera)
{
	myGraphicsPipeline.BindGraphics(aCommandBuffer);

	vkCmdBindDescriptorSets(aCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myPipelineLayout, 0, 1, &aDescriptorSet, 0, nullptr);
	
	std::vector<uint64_t> sortedPointLights;
	for (auto& kv : someEntites)
	{
		auto& entity = kv.second;
		if (!entity.HasPointLightComponent())
			continue;
		sortedPointLights.push_back(kv.first);
	}

	glm::vec3 camPos = aCamera.GetPosition();

	std::sort(sortedPointLights.begin(), sortedPointLights.end(), [&someEntites, &camPos](uint64_t Ida, uint64_t Idb) -> bool
	{
		glm::vec3 offset1 = camPos - someEntites.at(Ida).GetTransformComponent().myTranslation;
		glm::vec3 offset2 = camPos - someEntites.at(Idb).GetTransformComponent().myTranslation;
		return glm::dot(offset1, offset1) > glm::dot(offset2, offset2);
	});

	for (auto id : sortedPointLights)
	{
		auto& entity = someEntites.at(id);
		PointLightPushConstants push{};
		push.position = glm::vec4(entity.GetTransformComponent().myTranslation, 1.f);
		push.color = glm::vec4(entity.GetColor(), entity.GetPointLightIntensity());
		push.radius = 0.1f;

		vkCmdPushConstants(aCommandBuffer, myPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PointLightPushConstants), &push);

		vkCmdDraw(aCommandBuffer, 6, 1, 0, 0);
	}
}

void AM_PointLightRenderSystem::CreateGraphicsPipeline(VkRenderPass aRenderPass)
{
	AM_PipelineUtils::GraphicsInitializer graphicsInitializer{};
	AM_PipelineUtils::GetDefaultStates(graphicsInitializer);
	AM_PipelineUtils::EnableMultiSampleState(graphicsInitializer, myVkContext.maxMSAASamples);
	AM_PipelineUtils::EnableAlphaBlendState(graphicsInitializer);

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	AM_PipelineUtils::FillPiplineCreateInfo(pipelineInfo, graphicsInitializer);
	pipelineInfo.layout = myPipelineLayout;
	pipelineInfo.renderPass = aRenderPass;

	myGraphicsPipeline.CreatePipeline(
		"../data/shader_bytecode/pointlight.vert.spv",
		"../data/shader_bytecode/pointlight.frag.spv",
		pipelineInfo);
}

