#include "AM_VkRenderMethodBillboard.h"

#include "AM_Camera.h"
#include "AM_Entity.h"
#include "TempBuffer.h"

AM_VkRenderMethodBillboard::AM_VkRenderMethodBillboard(
	AM_VkContext& aVkContext,
	const VkRenderPass aRenderPass,
	const std::string& aVertexShaderPath,
	const std::string& aFragmentShaderPath,
	uint32_t aBindingDescriptionCount /*= 1*/,
	uint32_t anAttributeDescriptionCount /*= 1*/,
	const VkVertexInputBindingDescription* aBindingDescription /*= nullptr*/,
	const VkVertexInputAttributeDescription* anAttributeDescription /*= nullptr*/)
	: AM_VkRenderMethod<AM_VkRenderMethodBillboard>(
		aVkContext,
		aRenderPass,
		aVertexShaderPath,
		aFragmentShaderPath,
		aBindingDescriptionCount,
		anAttributeDescriptionCount,
		aBindingDescription,
		anAttributeDescription)
{
}

void AM_VkRenderMethodBillboard::CreatePipeline_Imp(AM_VkDescriptorSetLayoutBuilder& outBuilder, VkPushConstantRange& outRange, AM_PipelineUtils::GraphicsInitializer& outPipelineInitializer)
{
	AM_PipelineUtils::EnableAlphaBlendState(outPipelineInitializer);

	outBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1);
	outBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

	outRange.offset = 0;
	outRange.size = sizeof(PointLightPushConstants);
	outRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
}

void AM_VkRenderMethodBillboard::Render_Imp(VkCommandBuffer aCommandBuffer, VkDescriptorSet aDescriptorSet, std::unordered_map<uint64_t, AM_Entity>& someEntites, const AM_Camera& aCamera)
{
	myPipeline.BindGraphics(aCommandBuffer);

	vkCmdBindDescriptorSets(aCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myPipeline.GetPipelineLayout(), 0, 1, &aDescriptorSet, 0, nullptr);

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

		vkCmdPushConstants(aCommandBuffer, myPipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PointLightPushConstants), &push);

		vkCmdDraw(aCommandBuffer, 6, 1, 0, 0);
	}
}

