#pragma once
#include "TempBuffer.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vector>

struct PointLightData
{
	alignas(16) glm::vec4 color{};
	alignas(16) glm::vec4 position{};
};

struct GlobalUBO
{
	PointLightData pointLightData[8];
	alignas(16) glm::mat4 viewMat{};
	alignas(16) glm::mat4 projectionMat{};
	alignas(16) glm::mat4 invViewMat{};
	alignas(16) glm::vec4 ambientColor{};
	alignas(16) glm::vec3 directLightDirection{};
	alignas(4) int numPointLight{ 0 };
};

class AM_Camera;
class AM_EntityStorage;
class AM_TempScene
{
public:
	AM_TempScene()
		: myUBO{}
		, myUniformBuffer{}
		, myDescriptorSetLayout(nullptr)
		, myCamera(nullptr)
		, mySkybox(0)
		, myShouldUpdateUniformBuffer(0)
	{
	}

	AM_TempScene(const AM_TempScene&) = delete;
	AM_TempScene& operator=(const AM_TempScene&) = delete;

	void SetUniformBuffer(TempBuffer& aUniformBuffer) { myUniformBuffer = aUniformBuffer; }
	const TempBuffer* GetUniformBuffer() const { return &myUniformBuffer; }

	void SetDescriptorSetLayout(VkDescriptorSetLayout aLayout) { myDescriptorSetLayout = aLayout; }
	VkDescriptorSetLayout GetDescriptorSetLayout() const { return myDescriptorSetLayout; }

	const std::vector<VkDescriptorSet>& GetDescriptorSets() const { return myDescriptorSets; }
	std::vector<VkDescriptorSet>& GetDescriptorSets() { return myDescriptorSets; }

	GlobalUBO& GetUBO() { return myUBO; }
	const GlobalUBO& GetUBO() const { return myUBO; }

	void SetCamera(AM_Camera* aCamera) { myCamera = aCamera; }
	AM_Camera* GetCamera() { return myCamera; }

	void UpdateUBO_Camera();
	void UpdateUBO_PointLights(AM_EntityStorage& anEntityStorage);
	void UpdateUBO_DirectLighting(const glm::vec3& aLightDirection);
	void UpdateUBO_AmbientColor(const glm::vec4& aColor);

	void AddMeshObject(uint64_t anId);
	void AddPointLight(uint64_t anId);
	void AddSkybox(uint64_t anId);

	uint64_t GetSkyboxId() const { return mySkybox; }
	void ResetUpdateFlag(uint32_t aFrameIndex);
	bool GetShouldUpdateUniformBuffer(uint32_t aFrameIndex) const;

	const std::vector<uint64_t>& GetPointLights() const { return myPointLights; }

private:
	GlobalUBO myUBO;
	std::vector<VkDescriptorSet> myDescriptorSets;
	std::vector<uint64_t> myPointLights;
	std::vector<uint64_t> myMeshObjects;
	TempBuffer myUniformBuffer;
	VkDescriptorSetLayout myDescriptorSetLayout;
	AM_Camera* myCamera;
	uint64_t mySkybox;
	uint8_t myShouldUpdateUniformBuffer;
};