#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>
#include <vector>

struct Vertex
{
	glm::vec3 myPosition;
	glm::vec3 myColor;
	glm::vec3 myNormal;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription GetBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions();

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
	glm::vec3 myTranslation;
	glm::vec3 myScale{1.f,1.f,1.f};
	glm::vec3 myRotation{};

	glm::mat4 GetMatrix()
	{
		const float c3 = glm::cos(myRotation.z);
		const float s3 = glm::sin(myRotation.z);
		const float c2 = glm::cos(myRotation.x);
		const float s2 = glm::sin(myRotation.x);
		const float c1 = glm::cos(myRotation.y);
		const float s1 = glm::sin(myRotation.y);
		return glm::mat4
		{
			{
				myScale.x * (c1 * c3 + s1 * s2 * s3),
				myScale.x * (c2 * s3),
				myScale.x * (c1 * s2 * s3 - c3 * s1),
				0.0f,
			},
			{
				myScale.y * (c3 * s1 * s2 - c1 * s3),
				myScale.y * (c2 * c3),
				myScale.y * (c1 * c3 * s2 + s1 * s3),
				0.0f,
			},
			{
				myScale.z * (c2 * s1),
				myScale.z * (-s2),
				myScale.z * (c1 * c2),
				0.0f,
			},
			{
				myTranslation.x,
				myTranslation.y,
				myTranslation.z,
				1.0f
			}
		};
	}
};

class AM_Buffer;
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

	uint64_t GetId() const { return myId; }
	TransformComponent& GetTransformComponent() { return myTransform; }
	const TransformComponent& GetTransformComponent() const { return myTransform; }

	const std::vector<Vertex>& GetVertices() const { return myVertices; }
	const std::vector<uint32_t>& GetIndices() const { return myIndices; }
	AM_Buffer* GetVertexBuffer() const { return myVirtualVertexBuffer; }
	AM_Buffer* GetIndexBuffer() const { return myVirtualIndexBuffer; }

	void LoadModel();
	void SetVertexData(std::vector<Vertex>&& someVertices) { myVertices = std::move(someVertices); }
	void SetIndexData(std::vector<uint32_t>&& someIndices) { myIndices = std::move(someIndices); }
	void SetVertexBuffer(AM_Buffer* aVertexBuffer) { myVirtualVertexBuffer = aVertexBuffer; }
	void SetIndexBuffer(AM_Buffer* anIndexBuffer) { myVirtualIndexBuffer = anIndexBuffer; }

private:
	explicit AM_Entity(uint64_t anId)
		: myId(anId)
		, myTransform{}
	{
	}

	AM_Entity(const AM_Entity& anEntity) = delete;
	AM_Entity& operator=(const AM_Entity& anEntity) = delete;

	std::vector<Vertex> myVertices;
	std::vector<uint32_t> myIndices;
	AM_Buffer* myVirtualVertexBuffer;
	AM_Buffer* myVirtualIndexBuffer;
	uint64_t myId;
	TransformComponent myTransform;
};

