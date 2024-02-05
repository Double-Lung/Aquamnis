#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "TempBuffer.h"

struct Vertex
{
	glm::vec3 myPosition;
	glm::vec3 myColor;
	glm::vec3 myNormal;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription GetBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions();

	bool operator==(const Vertex& aVertex) const
	{
		return myPosition == aVertex.myPosition &&
			myColor == aVertex.myColor &&
			myNormal == aVertex.myNormal &&
			texCoord == aVertex.texCoord;
	}
};

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& aVertex) const
		{
			return ((hash<glm::vec3>()(aVertex.myPosition) ^
				(hash<glm::vec3>()(aVertex.myColor) << 1)) >> 1) ^
				(hash<glm::vec3>()(aVertex.myNormal) << 1) ^
				(hash<glm::vec2>()(aVertex.texCoord) << 1);
		}
	};
}

struct TransformComponent
{
	glm::vec3 myTranslation{ 0.f, 0.f, 0.f };
	glm::vec3 myScale{1.f,1.f,1.f};
	glm::vec3 myRotation{0.f, 0.f, 0.f};

	glm::mat4 GetNormalMatrix();
	glm::mat4 GetMatrix();
};

struct PointLightComponent
{
	float myIntensity;
};

class AM_Entity
{
public:
	AM_Entity(AM_Entity&&) = default;
	AM_Entity& operator=(AM_Entity&&) = default;

	static AM_Entity CreateEntity()
	{
		static uint64_t id = 0;
		return AM_Entity{id++};
	}

	void InitLightComponent(float anIntensity = 1.f)
	{
		myPointLightComponent = std::make_unique<PointLightComponent>();
		myPointLightComponent->myIntensity = anIntensity;
	}

	uint64_t GetId() const { return myId; }
	TransformComponent& GetTransformComponent() { return myTransform; }
	const TransformComponent& GetTransformComponent() const { return myTransform; }
	bool HasPointLightComponent() { return myPointLightComponent != nullptr; }
	float GetPointLightIntensity(){ return myPointLightComponent->myIntensity; }

	void SetIsCube(bool anIsCube) { myIsCube = anIsCube; }
	bool GetIsCube() { return myIsCube; }

	const std::vector<Vertex>& GetVertices() const { return myVertices; }
	const std::vector<uint32_t>& GetIndices() const { return myIndices; }
	const TempBuffer* GetTempVertexBuffer() const { return &myTempVertexBuffer; }
	const TempBuffer* GetTempIndexBuffer() const { return &myTempIndexBuffer; }
	
	void SetColor(const glm::vec3& aColor) { myColor = aColor; }
	const glm::vec3& GetColor() const { return myColor; }

	void LoadModel(const char* aFilePath);
	void SetVertexData(std::vector<Vertex>&& someVertices) { myVertices = std::move(someVertices); }
	void SetIndexData(std::vector<uint32_t>&& someIndices) { myIndices = std::move(someIndices); }

	void SetVertexBuffer(TempBuffer aVertexBuffer) { myTempVertexBuffer = aVertexBuffer; }
	void SetIndexBuffer(TempBuffer anIndexBuffer) { myTempIndexBuffer = anIndexBuffer; }

private:
	explicit AM_Entity(uint64_t anId);

	AM_Entity(const AM_Entity& anEntity) = delete;
	AM_Entity& operator=(const AM_Entity& anEntity) = delete;

	std::vector<Vertex> myVertices;
	std::vector<uint32_t> myIndices;
	TempBuffer myTempVertexBuffer;
	TempBuffer myTempIndexBuffer;

	uint64_t myId;
	TransformComponent myTransform;
	glm::vec3 myColor;
	std::unique_ptr<PointLightComponent> myPointLightComponent;

	bool myIsCube;
};

