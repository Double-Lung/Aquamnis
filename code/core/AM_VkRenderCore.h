#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include "AM_Entity.h"
#include "AM_NaiveMemoryAllocator.h"
#include "AM_VkBuffer.h"
#include "AM_VkImage.h"
#include "AM_VkSwapChain.h"
#include "AM_Window.h"
#include "AM_VkDescriptorUtils.h"
#include <array>
#include <glm/glm.hpp>
#include <string>

class AM_VkRenderer;
class AM_SimpleRenderSystem;
class AM_PointLightRenderSystem;
class AM_SimpleGPUParticleSystem;
class AM_Camera;

struct VmaAllocator_T;
typedef VmaAllocator_T* VmaAllocator;

class AM_VkRenderCore
{
public:
	// Create window and Vulkan instance
	void Engage();
	AM_VkRenderCore();
	~AM_VkRenderCore();
	
private:
	struct PointLightInstanceData
	{
		glm::vec4 position{};
		glm::vec4 color{};
	};

	struct UniformBufferObject
	{
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 inverseView;
		glm::vec4 ambientColor;
		PointLightInstanceData pointLightData[8];
		int numLights;
		float deltaTime;
	};

	bool CheckExtensionSupport();
	bool CheckInstanceLayerSupport();
	void CreateInstance();
	void CreateImageView(AM_VkImageView& outImageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t aMipLevels);
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateTextureImageView();
	AM_Image* CreateImage(const VkExtent2D& anExtent, uint32_t aMipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);

	void CreateTextureImage();
	void CopyBufferToImage(AM_Buffer& aBuffer, VkImage anImage, const uint32_t aWidth, const uint32_t aHeight, VkCommandBuffer aCommandBuffer);

	void CopyBuffer(AM_Buffer& aSourceBuffer, AM_Buffer& aDestinationBuffer, const VkDeviceSize aSize);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t aMipLevels, VkCommandBuffer aCommandBuffer);
	void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t aMipLevels);

	void CreateVertexBuffer(AM_Entity& anEntity);
	void CreateIndexBuffer(AM_Entity& anEntity);
	void CreateUniformBuffers();
	void UpdateUniformBuffer(uint32_t currentImage, const AM_Camera& aCamera, std::unordered_map<uint64_t, AM_Entity>& someEntites, float aDeltaTime);

	void CreateShaderStorageBuffers();

	void BeginOneTimeCommands(VkCommandBuffer& aCommandBuffer, VkCommandPool& aCommandPool);
	void EndOneTimeCommands(VkCommandBuffer commandBuffer, VkQueue aVkQueue, VkCommandPool aCommandPool);

	// also submit commands in source queue
	void BeginOwnershipTransfer(VkCommandBuffer& aSrcCommandBuffer, VkQueue& aSrcQueue, VkSemaphore& aSignalSemaphore);
	// also submit commands in destination queue
	void EndOwnershipTransfer(VkCommandBuffer& aDstCommandBuffer, VkQueue& aDstQueue, VkSemaphore& aWaitSemaphore);

	void CreateSyncObjects();
	void LoadEntities();

	void CreateTextureSampler();
	bool HasStencilComponent(VkFormat format);
	void InitVulkan();
	void MainLoop();

	void UpdateCameraTransform(float aDeltaTime, AM_Camera& aCamera);

	AM_Window myWindowInstance;
	AM_VkContext myVkContext;
	AM_NaiveMemoryAllocator myMemoryAllocator;
	VmaAllocator myVMA;

	std::vector<AM_VkSemaphore> myTransferSemaphores;
	std::vector<AM_Buffer*> myVirtualShaderStorageBuffers;

	AM_VkDescriptorPool myDescriptorPool;

	AM_Image* myTextureImage = nullptr;
	AM_VkImageView myTextureImageView;
	AM_VkSampler myTextureSampler;

	std::unordered_map<uint64_t, AM_Entity> myEntities;

	AM_Buffer* myVirtualUniformBuffer = nullptr;

	std::vector<VkDescriptorSet> myDescriptorSets;
	std::vector<VkDescriptorSet> myComputeDescriptorSets;
	
	uint32_t myMipLevels;
	AM_VkRenderer* myRenderer = nullptr;
	AM_SimpleRenderSystem* myRenderSystem = nullptr;
	AM_PointLightRenderSystem* myPointLightRenderSystem = nullptr;
	AM_SimpleGPUParticleSystem* mySimpleGPUParticleSystem = nullptr;
};

