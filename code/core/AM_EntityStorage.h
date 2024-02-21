#pragma once
#include "AM_Entity.h"
#include <unordered_map>

VK_DEFINE_HANDLE(VmaAllocator);
class AM_EntityStorage
{
public:
	AM_EntityStorage();
	~AM_EntityStorage();
	AM_Entity* Add();
	AM_Entity* GetIfExist(uint64_t anId);
	void GetEntitiesOfType(std::vector<AM_Entity*>& outEntities, AM_Entity::EntityType aType);
	void DestroyEntities(AM_VkContext& aVkContext, VmaAllocator anAllocator);
	const std::unordered_map<uint64_t, AM_Entity*>& GetStorage() const { return myEntities; }

private:
	std::unordered_map<uint64_t, AM_Entity*> myEntities;
};