#pragma once

#include "AM_VkSwapChain.h"
#include "TempBuffer.h"
#include "TempImage.h"
#include "AM_Entity.h"
#include "AM_VertexInfo.h"
#include "AM_Particle.h"
#include <glm/glm.hpp>
#include <array>
#include <string>

class AM_VkRenderContext;
class AM_VkRenderMethodMesh;
class AM_VkRenderMethodBillboard;
class AM_VkRenderMethodCubeMap;
class AM_VkRenderMethodPoint;
class AM_VkDescriptorSetWritesBuilder;
class AM_Camera;
class AM_Window;
class AM_EntityStorage;
class AM_TempScene;

struct VmaAllocationInfo;
VK_DEFINE_HANDLE(VmaAllocation);
VK_DEFINE_HANDLE(VmaAllocator);
class AM_VkRenderCore
{
public:
	void Setup();
	void Render(AM_Camera& aCamera, AM_TempScene& aScene, AM_EntityStorage& anEntityStorage);
	void OnEnd();
	void InitScene(AM_TempScene& aScene);
	AM_Entity* LoadSkybox(const char** someTexturePaths, AM_EntityStorage& anEntityStorage);
	AM_Entity* LoadEntity(const char** someTexturePaths, const char* aModelPath, AM_EntityStorage& anEntityStorage, AM_Entity::EntityType aType);

	explicit AM_VkRenderCore(AM_Window& aWindowInstance);
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

	VkDescriptorSetLayout GePerEntitytDescriptorSetLayout(const AM_Entity& anEntity);
	void GeneratePerEntityDescriptorInfo(AM_VkDescriptorSetWritesBuilder& outBuilder, const AM_Entity& anEntity, int aFrameNumber);
	void AllocatePerEntityUBO(AM_Entity& outEntity);
	void AllocatePerEntityDescriptorSets(AM_Entity& outEntity);

	void WriteEntityUniformBuffer(AM_Entity& anEntity);
	void WriteSceneUbiformBuffer(AM_TempScene& aScene);

	void BeginOneTimeCommands(VkCommandBuffer& aCommandBuffer, VkCommandPool& aCommandPool);
	void EndOneTimeCommands(VkCommandBuffer commandBuffer, VkQueue aVkQueue, VkCommandPool aCommandPool);

	// also submit commands in source queue
	void BeginOwnershipTransfer(VkCommandBuffer& aSrcCommandBuffer, VkQueue& aSrcQueue, VkSemaphore& aSignalSemaphore);
	// also submit commands in destination queue
	void EndOwnershipTransfer(VkCommandBuffer& aDstCommandBuffer, VkQueue& aDstQueue, VkSemaphore& aWaitSemaphore);
	
	bool HasStencilComponent(VkFormat format);
	void LoadModel(std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices, const char* aFilePath);
	void LoadVertexData(AM_Entity& outEntity, const char* aFilePath);

	AM_Window& myWindowInstance;
	AM_VkContext myVkContext;

	std::vector<VkSemaphore> myTransferSemaphores;
	VkDescriptorPool myGlobalDescriptorPool;

	uint32_t myMipLevels;
	VmaAllocator myVMA = nullptr;
	AM_VkRenderContext* myRenderContext = nullptr;
	AM_VkRenderMethodMesh* myMeshRenderMethod = nullptr;
	AM_VkRenderMethodBillboard* myBillboardRenderMethod = nullptr;
	AM_VkRenderMethodCubeMap* myCubeMapRenderMethod = nullptr;
};

