#pragma once
#include "TempBuffer.h"
#include "AM_Texture.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <memory>

class AM_EntityStorage;
class AM_Entity
{
	friend AM_EntityStorage;
public:
	AM_Entity(AM_Entity&& anEntity) noexcept
	{
		*this = std::move(anEntity);
	}

	AM_Entity& operator=(AM_Entity&& anEntity) noexcept
	{
		if (this == &anEntity)
			return *this;

		myId = std::exchange(anEntity.myId, 0);
		myTexture = anEntity.myTexture;
		myTempVertexBuffer = anEntity.myTempVertexBuffer;
		myTempIndexBuffer = anEntity.myTempIndexBuffer;
		myColor = anEntity.myColor;
		myTranslation = anEntity.myTranslation;
		myScale = anEntity.myScale;
		myRotation = anEntity.myRotation;
		myLightIntensity = anEntity.myLightIntensity;
		myIsSkybox = anEntity.myIsSkybox;
		return *this;
	}

	AM_Entity(const AM_Entity& anEntity) = delete;
	AM_Entity& operator=(const AM_Entity& anEntity) = delete;

	glm::mat4 GetNormalMatrix();
	glm::mat4 GetMatrix();
	
	uint64_t GetId() const { return myId; }

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
	const TempBuffer* GetTempVertexBuffer() const { return &myTempVertexBuffer; }
	const TempBuffer* GetTempIndexBuffer() const { return &myTempIndexBuffer; }

	glm::vec3 myTranslation{ 0.f, 0.f, 0.f };
	glm::vec3 myScale{ 1.f, 1.f, 1.f };
	glm::vec3 myRotation{ 0.f, 0.f, 0.f };

private:
	explicit AM_Entity(uint64_t anID);
	static AM_Entity* CreateEntity();
	void SetId(uint64_t anId) { myId = anId; }
	
	AM_Texture myTexture;
	TempBuffer myTempVertexBuffer;
	TempBuffer myTempIndexBuffer;
	glm::vec3 myColor;
	
	uint64_t myId;
	float myLightIntensity;
	bool myIsSkybox;
	bool myIsEmissive;
};

