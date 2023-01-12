#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include "VkDrawContext.h"
#include <array>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "AM_NaiveMemoryAllocator.h"
#include "AM_VkBuffer.h"
#include "AM_VkImage.h"
#include "AM_VkPrimitives.h"
#include "AM_VkSwapChain.h"

class VkDrawContext;
class VkDraw
{
public:
	// Create window and Vulkan instance
	void Engage();
	VkDraw() = default;
	~VkDraw();
	
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
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void CreateInstance();
	void InitWindow();

	void CreateSwapChain();

	void CleanupSwapChain();
	void RecreateSwapChain();
	void CreateImageView(AM_VkImageView& outImageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t aMipLevels);
	void CreateSwapChainImageViews();
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
	void Cleanup();

#ifdef _DEBUG
	void SetupDebugMessenger();
#endif

	static std::vector<char> ReadFile(const std::string& filename);
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

	GLFWwindow* myWindow = nullptr;
	VkDebugUtilsMessengerEXT myDebugMessenger = nullptr;
	AM_VkSwapChain mySwapChain;
	AM_VkRenderPass myRenderPass;
	AM_VkDescriptorSetLayout myDescriptorSetLayout;
	AM_VkPipelineLayout myPipelineLayout;
	AM_VkPipeline myGraphicsPipeline;
	std::vector<AM_VkCommandPool> myCommandPools;
	AM_VkCommandPool myTransferCommandPool;
	AM_VkDescriptorPool myDescriptorPool;
	std::vector<VkDescriptorSet> myDescriptorSets;

	AM_VkImageView myTextureImageView;
	AM_VkImageView myDepthImageView;
	AM_VkImageView myColorImageView;
	AM_VkSampler myTextureSampler;

	uint32_t myMipLevels = 0;

	void* myUniformBuffersMapped;

	std::vector<AM_VkSemaphore> myImageAvailableSemaphores;
	std::vector<AM_VkSemaphore> myRenderFinishedSemaphores;
	std::vector<AM_VkFence> myInFlightFences;

	std::vector<AM_VkFramebuffer> mySwapChainFramebuffers;
	std::vector<VkCommandBuffer> myCommandBuffers;

	uint32_t myCurrentFrame = 0;
	bool myIsFramebufferResized = false;

	std::vector<Vertex> myVertices;
	std::vector<uint32_t> myIndices;

	AM_NaiveMemoryAllocator myMemoryAllocator;
	VkDrawContext myVkContext;
	AM_VkBuffer myUniformBuffers;
	AM_VkBuffer myVertexBuffer;
	AM_VkBuffer myIndexBuffer;
	
	AM_VkImage myTextureImage;
	AM_VkImage myColorImage;
	AM_VkImage myDepthImage;
};

