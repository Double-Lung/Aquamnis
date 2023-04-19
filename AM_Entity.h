#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>

struct TransformComponent
{
	glm::vec3 myTranslation;
	glm::vec3 myScale;

	glm::mat4 GetScaleMat4()
	{
		return 
		{ 
			{ myScale.x, 0.f, 0.f, 0.f }, 
			{ 0.f, myScale.y, 0.f, 0.f },
			{ 0.f, 0.f, myScale.z, 0.f },
			{ 0.f, 0.f, 0.f, 1.f } 
		};
	}
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

	uint64_t GetId() const { return myId; }
	TransformComponent& GetTransformComponent() { return myTransform; }
	const TransformComponent& GetTransformComponent() const { return myTransform; }
private:
	explicit AM_Entity(uint64_t anId)
		: myId(anId)
		, myTransform{}
	{
	}

	AM_Entity(const AM_Entity& anEntity) = delete;
	AM_Entity& operator=(const AM_Entity& anEntity) = delete;
	uint64_t myId;
	TransformComponent myTransform;
};

