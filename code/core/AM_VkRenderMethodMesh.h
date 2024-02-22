#pragma once
#include "AM_VkRenderMethod.h"
#include <glm/glm.hpp>

class AM_Camera;
class AM_Entity;
struct TempBuffer;
class AM_VkRenderMethodMesh : public AM_VkRenderMethod<AM_VkRenderMethodMesh>
{
public:
	AM_VkRenderMethodMesh(
		AM_VkContext& aVkContext,
		const VkRenderPass aRenderPass,
		const std::string& aVertexShaderPath,
		const std::string& aFragmentShaderPath,
		VkDescriptorSetLayout aGlobalLayout,
		uint32_t aBindingDescriptionCount = 1,
		uint32_t anAttributeDescriptionCount = 1,
		const VkVertexInputBindingDescription* aBindingDescription = nullptr,
		const VkVertexInputAttributeDescription* anAttributeDescription = nullptr);

	AM_VkRenderMethodMesh(const AM_VkRenderMethodMesh&) = delete;
	AM_VkRenderMethodMesh& operator=(const AM_VkRenderMethodMesh&) = delete;

	void CreatePipeline_Imp(AM_VkDescriptorSetLayoutBuilder& outBuilder, VkPushConstantRange& outRange, AM_PipelineUtils::GraphicsInitializer& outPipelineInitializer);
	void Render_Imp(AM_FrameRenderInfo& someInfo, std::vector<AM_Entity*>& someEntities, const TempBuffer* aBuffer);

private:
	struct PushConstantData
	{
		glm::mat4 normalMat;
		glm::mat4 transform;
	};
};