#pragma once
#include "AM_NaiveMemoryAllocator.h"
#include "AM_VkBuffer.h"
#include "AM_VkImage.h"
#include "AM_VkSwapChain.h"
#include "AM_Window.h"
#include <array>
#include <glm/glm.hpp>
#include <string>

class VkDraw
{
public:
	// Create window and Vulkan instance
	void Engage();
	VkDraw();
	~VkDraw() = default;
	
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
	void CreateImage(const VkExtent2D& anExtent, uint32_t aMipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, AM_VkImage& anImageObject);

	void CreateTextureImage();
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, AM_VkBuffer& aBufferObject);

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t aMipLevels);
	void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t aMipLevels);
	void CreateColorResources();

	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffers();
	void UpdateUniformBuffer(uint32_t currentImage);

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
	VkDrawContext myVkContext;
	AM_NaiveMemoryAllocator myMemoryAllocator;

	std::vector<AM_VkSemaphore> myImageAvailableSemaphores;
	std::vector<AM_VkSemaphore> myRenderFinishedSemaphores;
	std::vector<AM_VkFence> myInFlightFences;

	AM_VkSwapChain mySwapChain;
	std::vector<AM_VkCommandPool> myCommandPools;
	AM_VkCommandPool myTransferCommandPool;
	AM_VkDescriptorPool myDescriptorPool;
	AM_VkRenderPass myRenderPass;
	AM_VkDescriptorSetLayout myDescriptorSetLayout;
	AM_VkImage myColorImage;
	AM_VkImageView myColorImageView;
	AM_VkImage myDepthImage;
	AM_VkImageView myDepthImageView;
	AM_VkBuffer myUniformBuffers;
	AM_VkImage myTextureImage; 
	AM_VkImageView myTextureImageView;
	AM_VkSampler myTextureSampler;
	std::vector<Vertex> myVertices;
	std::vector<uint32_t> myIndices;
	AM_VkBuffer myVertexBuffer;
	AM_VkBuffer myIndexBuffer;
	std::vector<VkCommandBuffer> myCommandBuffers;
	std::vector<AM_VkFramebuffer> myFramebuffers;
	AM_VkPipelineLayout myPipelineLayout;
	AM_VkPipeline myGraphicsPipeline;
	std::vector<VkDescriptorSet> myDescriptorSets;
	
	void* myUniformBuffersMapped = nullptr; // unmap?
	uint32_t myMipLevels = 0;
	uint32_t myCurrentFrame = 0;
	bool myIsFramebufferResized = false;
};

