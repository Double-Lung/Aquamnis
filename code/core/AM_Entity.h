#pragma once
#include "AM_Texture.h"
#include "AM_VkContext.h"
#include "TempBuffer.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

class AM_EntityStorage;
class AM_Entity
{
	friend AM_EntityStorage;
public:
	enum EntityType : uint8_t
	{
		MESH,
		SKINNEDMESH,
		PARTICLE,
		BILLBOARD,
		SKYBOX,
		POINT,
		LINE,
	};

	struct EntityUBO
	{
		glm::mat4 transform{ 1.f };
		glm::mat4 transform2{ 1.f };
		glm::mat4 transform3{ 1.f };
		glm::mat4 transform4{ 1.f };
	};
	AM_Entity(AM_Entity&& anEntity) noexcept
	{
		*this = std::move(anEntity);
	}

	AM_Entity(const AM_Entity& anEntity) = delete;
	AM_Entity& operator=(const AM_Entity& anEntity) = delete;
	
	glm::mat4 GetNormalMatrix();
	glm::mat4 GetMatrix();
	
	uint64_t GetId() const { return myId; }
	EntityType GetType() const { return myType; }
	void SetType(EntityType aType) { myType = aType; }

	void SetIsEmissive(bool anIsEmissive) { myIsEmissive = anIsEmissive; }
	bool IsEmissive() const { return myIsEmissive; }

	void SetLightIntensity(float anIntensity = 1.f) { myLightIntensity = anIntensity; }
	float GetLightIntensity() const { return myLightIntensity; }

	void SetIsSkybox(bool anIsSkybox) { myIsSkybox = anIsSkybox; }
	bool GetIsSkybox() const { return myIsSkybox; }

	void SetColor(const glm::vec3& aColor) { myColor = aColor; }
	const glm::vec3& GetColor() const { return myColor; }

	AM_Texture& GetTexture() { return myTexture; }
	const AM_Texture& GetTexture() const { return myTexture; }

	void SetVertexBuffer(TempBuffer aVertexBuffer) { myTempVertexBuffer = aVertexBuffer; }
	void SetIndexBuffer(TempBuffer anIndexBuffer) { myTempIndexBuffer = anIndexBuffer; }
	void SetUniformBuffer(TempBuffer aUniformBuffer) { myTempUniformBuffer = aUniformBuffer; }
	const TempBuffer* GetTempVertexBuffer() const { return &myTempVertexBuffer; }
	const TempBuffer* GetTempIndexBuffer() const { return &myTempIndexBuffer; }
	const TempBuffer* GetUniformBuffer() const { return &myTempUniformBuffer; }

	const std::vector<VkDescriptorSet>& GetDescriptorSets() const { return myDescriptorSets; }
	std::vector<VkDescriptorSet>& GetDescriptorSets()  { return myDescriptorSets; }

	void SetTransparency(bool anIsTransparent) { myIsTransparent = anIsTransparent; }
	bool IsTransparent() const { return myIsTransparent; }

	void SetIndexBufferSize(uint32_t aSize) { myIndexBufferSize = aSize; }
	uint32_t GetIndexBufferSize() const { return myIndexBufferSize; }

	glm::vec3 myTranslation{ 0.f, 0.f, 0.f };
	glm::vec3 myScale{ 1.f, 1.f, 1.f };
	glm::vec3 myRotation{ 0.f, 0.f, 0.f };

private:
	AM_Entity& operator=(AM_Entity&& anEntity) noexcept
	{
		if (this == &anEntity)
			return *this;

		myId = std::exchange(anEntity.myId, 0);
		myDescriptorSets = std::move(anEntity.myDescriptorSets);
		myTexture = anEntity.myTexture;
		myTempVertexBuffer = anEntity.myTempVertexBuffer;
		myTempIndexBuffer = anEntity.myTempIndexBuffer;
		myTempUniformBuffer = anEntity.myTempUniformBuffer;
		myColor = anEntity.myColor;
		myTranslation = anEntity.myTranslation;
		myScale = anEntity.myScale;
		myRotation = anEntity.myRotation;
		myLightIntensity = anEntity.myLightIntensity;
		myIndexBufferSize = anEntity.myIndexBufferSize;
		myType = anEntity.myType;
		myIsSkybox = anEntity.myIsSkybox;
		myIsEmissive = anEntity.myIsEmissive;
		myIsTransparent = anEntity.myIsTransparent;
		return *this;
	}

	explicit AM_Entity(uint64_t anID);
	static AM_Entity* CreateEntity();
	void SetId(uint64_t anId) { myId = anId; }
	
	AM_Texture myTexture;
	std::vector<VkDescriptorSet> myDescriptorSets;
	TempBuffer myTempVertexBuffer;
	TempBuffer myTempIndexBuffer;
	TempBuffer myTempUniformBuffer;
	glm::vec3 myColor;
	uint64_t myId;
	float myLightIntensity;
	uint32_t myIndexBufferSize;
	EntityType myType;
	bool myIsSkybox : 1;
	bool myIsEmissive : 1;
	bool myIsTransparent : 1;
};

