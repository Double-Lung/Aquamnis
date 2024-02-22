#pragma once
#include "AM_Particle.h"
#include "AM_Texture.h"
#include "AM_VertexInfo.h"
#include "AM_VkContext.h"
#include <glm/glm.hpp>
#include <array>
#include <string>

class AM_Camera;
class AM_Entity;
class AM_EntityStorage;
class AM_TempScene;
class AM_VkDescriptorSetWritesBuilder;
class AM_VkRenderContext;
class AM_VkRenderMethodBillboard;
class AM_VkRenderMethodCubeMap;
class AM_VkRenderMethodMesh;
class AM_VkRenderMethodPoint;
class AM_Window;
struct TempBuffer;
struct TempImage;
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
	AM_Entity* LoadEntity(const char** someTexturePaths, const char* aModelPath, AM_EntityStorage& anEntityStorage, uint8_t aType);
	void DestroyEntities(AM_EntityStorage& anEntityStorage);
	void DestroyScene(AM_TempScene& aScene);

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

	void CreateTextureImage(TempImage& outImage, const char** somePaths, uint32_t aLayerCount = 1, VkImageCreateFlags someFlags = 0);
	void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t aMipLevels);

	void CopyBufferToImage(VkBuffer aSourceBuffer, VkImage anImage, uint32_t aWidth, uint32_t aHeight, uint32_t aLayerCount, VkCommandBuffer aCommandBuffer);
	void CopyBuffer(VkBuffer aSourceBuffer, VmaAllocationInfo* anAllocationInfo, const TempBuffer* aDestinationBuffer);

	void CreateFilledStagingBuffer(TempBuffer& outBuffer, uint64_t aBufferSize, uint64_t aStrideSize, std::vector<void*>& someSources);
	void UploadToBuffer(uint64_t aBufferSize, void* aSource, const TempBuffer* aBuffer);
	void CreateVertexBuffer(AM_Entity& outEntity, std::vector<Vertex>& someVertices);
	void CreateIndexBuffer(AM_Entity& outEntity, std::vector<uint32_t>& someIndices);

	VkDescriptorSetLayout GePerEntitytDescriptorSetLayout(const AM_Entity& anEntity);
	void AllocatePerEntityUBO(AM_Entity& outEntity);
	void AllocatePerEntityDescriptorSets(AM_Entity& outEntity);

	void WriteEntityUniformBuffer(AM_Entity& anEntity);
	void WriteSceneUbiformBuffer(AM_TempScene& aScene);

	// also submit commands in source queue
	void BeginOwnershipTransfer(VkCommandBuffer& aSrcCommandBuffer, VkQueue& aSrcQueue, VkSemaphore& aSignalSemaphore);
	// also submit commands in destination queue
	void EndOwnershipTransfer(VkCommandBuffer& aDstCommandBuffer, VkQueue& aDstQueue, VkSemaphore& aWaitSemaphore);

	void LoadModel(std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices, const char* aFilePath);
	void LoadVertexData(AM_Entity& outEntity, const char* aFilePath);
	void CreateRenderMethods(VkDescriptorSetLayout aGlobalLayout);
	void LoadDefaultTexture();

	AM_Window& myWindowInstance;
	AM_VkContext myVkContext;
	AM_Texture myDefaultTexture;
	std::vector<VkSemaphore> myTransferSemaphores;
	VkDescriptorPool myGlobalDescriptorPool;
	VmaAllocator myVMA;
	AM_VkRenderContext* myRenderContext;
	AM_VkRenderMethodMesh* myMeshRenderMethod;
	AM_VkRenderMethodBillboard* myBillboardRenderMethod;
	AM_VkRenderMethodCubeMap* myCubeMapRenderMethod;
	uint32_t myMipLevels;
};


