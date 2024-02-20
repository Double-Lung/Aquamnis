#pragma once

#include "AM_VkSwapChain.h"
#include "AM_Window.h"
#include "TempBuffer.h"
#include "TempImage.h"
#include <glm/glm.hpp>
#include <array>
#include <string>

class AM_VkRenderContext;
class AM_VkRenderMethodMesh;
class AM_VkRenderMethodBillboard;
class AM_VkRenderMethodCubeMap;
class AM_VkRenderMethodPoint;
class AM_Camera;
class AM_Entity;
class AM_EntityStorage;

struct VmaAllocationInfo;
VK_DEFINE_HANDLE(VmaAllocation);
VK_DEFINE_HANDLE(VmaAllocator);
class AM_VkRenderCore
{
public:
	void Setup();
	void MainLoop();
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
	void CreateImageView(VkImageView& outImageView, VkImage image, VkFormat format, VkImageViewType aViewType, VkImageAspectFlags aspectFlags, uint32_t aMipLevels, uint32_t aLayerCount);
	void AllocatePerEntityDescriptorSets(AM_Entity& outEntity);
	void CreateTextureImage(TempImage& outImage, const char** somePaths, uint32_t aLayerCount = 1);
	void CreateTextureSampler(VkSampler& outSampler, VkSamplerAddressMode anAddressMode, VkBorderColor aBorderColor, VkCompareOp aCompareOp);
	void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t aMipLevels);

	void CopyBufferToImage(VkBuffer aSourceBuffer, VkImage anImage, uint32_t aWidth, uint32_t aHeight, uint32_t aLayerCount, VkCommandBuffer aCommandBuffer);
	void CopyBuffer(VkBuffer aSourceBuffer, VmaAllocationInfo* anAllocationInfo, const TempBuffer* aDestinationBuffer);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t aMipLevels, uint32_t aLayerCount, VkCommandBuffer aCommandBuffer);

	void CreateFilledStagingBuffer(TempBuffer& outBuffer, uint64_t aBufferSize, uint64_t aStrideSize, std::vector<void*>& someSources);
	void UploadToBuffer(uint64_t aBufferSize, void* aSource, const TempBuffer* aBuffer);
	void CreateVertexBuffer(AM_Entity& outEntity, std::vector<Vertex>& someVertices);
	void CreateIndexBuffer(AM_Entity& outEntity, std::vector<uint32_t>& someIndices);

	void AllocatePerEntityUBO(AM_Entity& outEntity);
	void UpdateUniformBuffer(uint32_t currentImage, const AM_Camera& aCamera, std::unordered_map<uint64_t, AM_Entity>& someEntites, float aDeltaTime);

	void BeginOneTimeCommands(VkCommandBuffer& aCommandBuffer, VkCommandPool& aCommandPool);
	void EndOneTimeCommands(VkCommandBuffer commandBuffer, VkQueue aVkQueue, VkCommandPool aCommandPool);

	// also submit commands in source queue
	void BeginOwnershipTransfer(VkCommandBuffer& aSrcCommandBuffer, VkQueue& aSrcQueue, VkSemaphore& aSignalSemaphore);
	// also submit commands in destination queue
	void EndOwnershipTransfer(VkCommandBuffer& aDstCommandBuffer, VkQueue& aDstQueue, VkSemaphore& aWaitSemaphore);
	
	bool HasStencilComponent(VkFormat format);
	void LoadModel(std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices, const char* aFilePath);
	void LoadVertexData(AM_Entity& outEntity, const char* aFilePath);

	void LoadDefaultResources();

	void UpdateCameraTransform(float aDeltaTime, AM_Camera& aCamera);

	AM_Window myWindowInstance;
	AM_VkContext myVkContext;

	std::vector<VkSemaphore> myTransferSemaphores;

	VkDescriptorPool myGlobalDescriptorPool;

	AM_EntityStorage* myEntityStorage = nullptr;
	
	uint32_t myMipLevels;
	uint32_t myCubeMapMipLevels;
	VmaAllocator myVMA = nullptr;
	AM_VkRenderContext* myRenderContext = nullptr;
	AM_VkRenderMethodMesh* myMeshRenderMethod = nullptr;
	AM_VkRenderMethodBillboard* myBillboardRenderMethod = nullptr;
	AM_VkRenderMethodCubeMap* myCubeMapRenderMethod = nullptr;
};

