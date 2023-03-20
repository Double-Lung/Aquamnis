#pragma once
#include "AM_NaiveMemoryAllocator.h"
#include "AM_VkBuffer.h"
#include "AM_VkImage.h"
#include "AM_VkSwapChain.h"
#include "AM_Window.h"
#include <array>
#include <glm/glm.hpp>
#include <string>

class AM_VkRenderCore
{
public:
	// Create window and Vulkan instance
	void Engage();
	AM_VkRenderCore();
	~AM_VkRenderCore() = default;
	
private:
	struct Vertex {
		glm::vec3 myPosition;
		glm::vec3 myColor;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription GetBindingDescription();
		static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions();
	};

	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
		
	};

	bool CheckExtensionSupport();
	bool CheckInstanceLayerSupport();
	void CreateInstance();
	void CreateSwapChain();
	void CleanupSwapChain();
	void RecreateSwapChain();
	void CreateImageView(AM_VkImageView& outImageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t aMipLevels);
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	void CreateFramebuffers();
	void CreateCommandPools();
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateTextureImageView();
	AM_Image* CreateImage(const VkExtent2D& anExtent, uint32_t aMipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);

	void CreateTextureImage();
	void CopyBufferToImage(AM_Buffer& aBuffer, VkImage anImage, const uint32_t aWidth, const uint32_t aHeight, VkCommandBuffer aCommandBuffer);

	void CopyBuffer(AM_Buffer& aSourceBuffer, AM_Buffer& aDestinationBuffer, const VkDeviceSize aSize);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t aMipLevels, VkCommandBuffer aCommandBuffer);
	void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t aMipLevels);
	void CreateColorResources();

	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffers();
	void UpdateUniformBuffer(uint32_t currentImage);

	void BeginOneTimeCommands(VkCommandBuffer& aCommandBuffer, VkCommandPool& aCommandPool);
	// also submit commands in source queue
	void BeginOwnershipTransfer(VkCommandBuffer& aSrcCommandBuffer, VkQueue& aSrcQueue, VkSemaphore& aSignalSemaphore);
	// also submit commands in destination queue
	void EndOwnershipTransfer(VkCommandBuffer& aDstCommandBuffer, VkQueue& aDstQueue, VkSemaphore& aWaitSemaphore);
	void EndOneTimeCommands(VkCommandBuffer commandBuffer, VkQueue aVkQueue, VkCommandPool aCommandPool);

	VkCommandBuffer BeginSingleTimeCommands(VkCommandPool aCommandPool);
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool aCommandPool, VkQueue aVkQueue);

	void CreateReusableCommandBuffers();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, const uint32_t imageIndex);
	void CreateSyncObjects();
	void DrawFrame();

	void LoadModel();

	void CreateTextureSampler();
	void CreateDepthResources();
	bool HasStencilComponent(VkFormat format);
	void InitVulkan();
	void CreateRenderPass();
	void MainLoop();

	static std::vector<char> ReadFile(const std::string& filename);
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

	AM_Window myWindowInstance;
	AM_VkContext myVkContext;
	AM_NaiveMemoryAllocator myMemoryAllocator;

	std::vector<AM_VkSemaphore> myImageAvailableSemaphores;
	std::vector<AM_VkSemaphore> myRenderFinishedSemaphores;
	std::vector<AM_VkSemaphore> myTransferSemaphores;
	std::vector<AM_VkFence> myInFlightFences;
	std::vector<AM_VkFence> mySyncFences;

	AM_VkSwapChain mySwapChain;
	std::vector<AM_VkCommandPool> myCommandPools;
	AM_VkCommandPool myTransferCommandPool;
	AM_VkDescriptorPool myDescriptorPool;
	AM_VkRenderPass myRenderPass;
	AM_VkDescriptorSetLayout myDescriptorSetLayout;

	AM_Image* myColorImage = nullptr;
	AM_Image* myDepthImage = nullptr;
	AM_Image* myTextureImage = nullptr;
	AM_VkImageView myColorImageView;
	AM_VkImageView myDepthImageView;
	AM_VkImageView myTextureImageView;

	AM_VkSampler myTextureSampler;
	std::vector<Vertex> myVertices;
	std::vector<uint32_t> myIndices;

	AM_Buffer* myVirtualUniformBuffer = nullptr;
	AM_Buffer* myVirtualVertexBuffer = nullptr;
	AM_Buffer* myVirtualIndexBuffer = nullptr;

	std::vector<VkCommandBuffer> myCommandBuffers;
	std::vector<AM_VkFramebuffer> myFramebuffers;
	AM_VkPipelineLayout myPipelineLayout;
	AM_VkPipeline myGraphicsPipeline;
	std::vector<VkDescriptorSet> myDescriptorSets;
	
	uint32_t myMipLevels;
	uint32_t myCurrentFrame;
	bool myIsFramebufferResized;
};

