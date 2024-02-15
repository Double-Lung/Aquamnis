#pragma once
#include "AM_VkRenderMethod.h"
#include <glm/glm.hpp>

struct TempBuffer;
class AM_Entity;
class AM_Camera;

class AM_VkRenderMethodMesh : public AM_VkRenderMethod<AM_VkRenderMethodMesh>
{
public:
	AM_VkRenderMethodMesh(
		AM_VkContext& aVkContext,
		const VkRenderPass aRenderPass,
		const std::string& aVertexShaderPath,
		const std::string& aFragmentShaderPath,
		uint32_t aBindingDescriptionCount = 1,
		uint32_t anAttributeDescriptionCount = 1,
		const VkVertexInputBindingDescription* aBindingDescription = nullptr,
		const VkVertexInputAttributeDescription* anAttributeDescription = nullptr);

	AM_VkRenderMethodMesh(const AM_VkRenderMethodMesh&) = delete;
	AM_VkRenderMethodMesh& operator=(const AM_VkRenderMethodMesh&) = delete;

	void CreatePipeline_Imp(AM_VkDescriptorSetLayoutBuilder& outBuilder, VkPushConstantRange& outRange, AM_PipelineUtils::GraphicsInitializer& outPipelineInitializer);
	void Render_Imp(VkCommandBuffer aCommandBuffer, VkDescriptorSet aDescriptorSet, const TempBuffer* aBuffer, const AM_Camera& aCamera);
	void Render_Imp(VkCommandBuffer aCommandBuffer, VkDescriptorSet aDescriptorSet, std::unordered_map<uint64_t, AM_Entity>& someEntites, const AM_Camera& aCamera);

private:
	struct PushConstantData
	{
		glm::mat4 normalMat;
		glm::mat4 transform;
	};
};