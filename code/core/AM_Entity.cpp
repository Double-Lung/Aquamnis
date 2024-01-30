#define TINYOBJLOADER_IMPLEMENTATION
#include "AM_Entity.h"
#include "AM_VkRenderCoreConstants.h"
#include <array>
#include <stdexcept>
#include <tiny_obj_loader.h>
#include <unordered_map>

VkVertexInputBindingDescription Vertex::GetBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 4> Vertex::GetAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, myPosition);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, myColor);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, myNormal);

	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

	return attributeDescriptions;
}

void AM_Entity::LoadModel(const char* aFilePath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, aFilePath))
		throw std::runtime_error(warn + err);

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const tinyobj::shape_t& shape : shapes)
	{
		for (const tinyobj::index_t& index : shape.mesh.indices)
		{
			Vertex vertex{};
			vertex.myPosition =
			{
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};
			vertex.myNormal =
			{
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};
			vertex.texCoord =
			{
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.f - attrib.texcoords[2 * index.texcoord_index + 1] // vulkan uv
			};
			vertex.myColor = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(myVertices.size());
				myVertices.push_back(vertex);
			}

			myIndices.push_back(uniqueVertices[vertex]);
		}
	}
}

AM_Entity::AM_Entity(uint64_t anId) 
	: myTempVertexBuffer{}
	, myTempIndexBuffer{}
	, myId(anId)
	, myTransform{}
	, myColor{ 1.f, 1.f, 1.f }
	, myPointLightComponent{ nullptr }
{
}

glm::mat4 TransformComponent::GetNormalMatrix()
{
	const float c3 = glm::cos(myRotation.z);
	const float s3 = glm::sin(myRotation.z);
	const float c2 = glm::cos(myRotation.x);
	const float s2 = glm::sin(myRotation.x);
	const float c1 = glm::cos(myRotation.y);
	const float s1 = glm::sin(myRotation.y);
	const glm::vec3 invScale = 1.f / myScale;
	return glm::mat4
	{
		{
			invScale.x * (c1 * c3 + s1 * s2 * s3),
			invScale.x * (c2 * s3),
			invScale.x * (c1 * s2 * s3 - c3 * s1),
			0.f
		},
		{
			invScale.y * (c3 * s1 * s2 - c1 * s3),
			invScale.y * (c2 * c3),
			invScale.y * (c1 * c3 * s2 + s1 * s3),
			0.f
		},
		{
			invScale.z * (c2 * s1),
			invScale.z * (-s2),
			invScale.z * (c1 * c2),
			0.f
		},
		{
			0.f,0.f,0.f,1.f
		}
	};
}

glm::mat4 TransformComponent::GetMatrix()
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
