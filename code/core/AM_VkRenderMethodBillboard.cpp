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

void AM_VkRenderMethodBillboard::Render_Imp(AM_FrameRenderInfo& someInfo, std::vector<AM_Entity*>& someEntities, const TempBuffer* /*aBuffer*/)
{
	VkCommandBuffer commandBuffer = someInfo.myCommandBuffer;
	myPipeline.BindGraphics(commandBuffer);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myPipeline.GetPipelineLayout(), 0, 1, &someInfo.myGlobalDescriptorSet, 0, nullptr);

	std::vector<AM_Entity*> sortedEntities;
	std::vector<AM_Entity*> normalEntities;
	for (AM_Entity* entity : someEntities)
	{
		if (entity->IsTransparent())
			sortedEntities.push_back(entity);
		else
			normalEntities.push_back(entity);
	}

	glm::vec3 camPos = someInfo.myCamera->GetPosition();
	std::sort(sortedEntities.begin(), sortedEntities.end(), [&camPos](AM_Entity* lh, AM_Entity* rh) -> bool
	{
		glm::vec3 offset1 = camPos - lh->myTranslation;
		glm::vec3 offset2 = camPos - rh->myTranslation;
		return glm::dot(offset1, offset1) > glm::dot(offset2, offset2);
	});

	// draw opaque first
	for (AM_Entity* entity : normalEntities)
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myPipeline.GetPipelineLayout(), 1, 1, &entity->GetDescriptorSets()[someInfo.myFrameIndex], 0, nullptr);
// 		PointLightPushConstants push{};
// 		push.position = glm::vec4(entity.GetTransformComponent().myTranslation, 1.f);
// 		push.color = glm::vec4(entity.GetColor(), entity.GetPointLightIntensity());
// 		push.radius = 0.1f;
		vkCmdDraw(commandBuffer, 6, 1, 0, 0);  // draw 6 vertices for a quad
	}

	for (AM_Entity* entity : sortedEntities)
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, myPipeline.GetPipelineLayout(), 1, 1, &entity->GetDescriptorSets()[someInfo.myFrameIndex], 0, nullptr);
// 		PointLightPushConstants push{};
// 		push.position = glm::vec4(entity.GetTransformComponent().myTranslation, 1.f);
// 		push.color = glm::vec4(entity.GetColor(), entity.GetPointLightIntensity());
// 		push.radius = 0.1f;
		vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	}
}

